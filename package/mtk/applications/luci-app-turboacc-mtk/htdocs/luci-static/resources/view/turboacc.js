/* Copyright (C) 2022 ImmortalWrt.org */

'use strict';
'require form';
'require fs';
'require poll';
'require rpc';
'require uci';
'require view';

var getSystemFeatures = rpc.declare({
	object: 'luci.turboacc',
	method: 'getSystemFeatures',
	expect: { '': {} }
});

var getFastPathStat = rpc.declare({
	object: 'luci.turboacc',
	method: 'getFastPathStat',
	expect: { '': {} }
});

var getFullConeStat = rpc.declare({
	object: 'luci.turboacc',
	method: 'getFullConeStat',
	expect: { '': {} }
});

var getTCPCCAStat = rpc.declare({
	object: 'luci.turboacc',
	method: 'getTCPCCAStat',
	expect: { '': {} }
});

var getMTKPPEStat = rpc.declare({
	object: 'luci.turboacc',
	method: 'getMTKPPEStat',
	expect: { '': {} }
});

function progressbar(value, max, byte) {
	var vn = parseInt(value) || 0,
		mn = parseInt(max) || 100,
		fv = byte ? String.format('%1024.2mB', value) : value,
		fm = byte ? String.format('%1024.2mB', max) : max,
		pc = Math.floor((100 / mn) * vn);

	return E('div', {
		'class': 'cbi-progressbar',
		'title': '%s / %s (%d%%)'.format(fv, fm, pc)
	}, E('div', { 'style': 'width:%.2f%%'.format(pc) }));
}

function formatCPUUsage(stats) {
	var text = stats.CPU_USED || '0%';

	if (stats.CPU_CORES_USED) {
		if (Array.isArray(stats.CPU_CORES_USED)) {
			if (stats.CPU_CORES_USED.length > 0)
				text += ' (' + stats.CPU_CORES_USED.join(', ') + ')';
		} else {
			var cores = [];
			for (var core in stats.CPU_CORES_USED) {
				if (stats.CPU_CORES_USED.hasOwnProperty(core))
					cores.push(core);
			}
			cores.sort(function (a, b) {
				var na = parseInt(a.replace(/^CPU/i, ''), 10);
				var nb = parseInt(b.replace(/^CPU/i, ''), 10);
				return na - nb;
			});
			var coreItems = [];
			for (var i = 0; i < cores.length; i++) {
				var k = cores[i];
				coreItems.push(k + ': ' + stats.CPU_CORES_USED[k]);
			}
			if (coreItems.length > 0)
				text += ' (' + coreItems.join(', ') + ')';
		}
	}

	return text;
}

function renderStatus(stats) {
	var spanTemp = '<em><span style="color:%s"><strong>%s</strong></span></em>';
	var renderHTML = [];
	for (var stat of stats)
		if (stat.type) {
			if (stat.type.includes(' / ')) {
				var types = stat.type.split(' / ');
				var inner = spanTemp.format('green', types[0]);
				for (var i of types.slice(1))
					inner += spanTemp.format('none', ' / ') + spanTemp.format('red', i);
				renderHTML.push(inner);
			} else
				renderHTML.push(spanTemp.format('green', stat.type));
		} else
			renderHTML.push(spanTemp.format('red', _('Disabled')));
	return renderHTML;
}

return view.extend({
	load: function() {
		return Promise.all([
			uci.load('turboacc'),
			L.resolveDefault(getSystemFeatures(), {}),
			L.resolveDefault(getMTKPPEStat(), {}),
			L.resolveDefault(getFastPathStat(), {}),
			L.resolveDefault(getFullConeStat(), {}),
			L.resolveDefault(getTCPCCAStat(), {})
		]);
	},

	render: function(data) {
		var m, s, o;
		var features = data[1];
		var ppe_stats = data[2];
		var svc_stats  = [ data[3], data[4], data[5] ];

		m = new form.Map('turboacc', _('TurboACC settings'),
			_('Open source flow offloading engine (fast path or hardware NAT).'));

		s = m.section(form.TypedSection);
		s.anonymous = true;
		s.render = function () {
			/* ---- single unified poll: status + ppe/cpu/conntrack ---- */
			poll.add(function () {
				return Promise.all([
					L.resolveDefault(getFastPathStat(), {}),
					L.resolveDefault(getFullConeStat(), {}),
					L.resolveDefault(getTCPCCAStat(), {}),
					L.resolveDefault(getMTKPPEStat(), {})
				]).then(function (res) {
					var svc = [res[0], res[1], res[2]];
					var stats = renderStatus(svc);
					var tds = [ 'fastpath_state', 'fullcone_state', 'tcpcca_state' ];
					for (var i in tds) {
						var view = document.getElementById(tds[i]);
						if (view) view.innerHTML = stats[i];
					}

					var stat = res[3] || {};
					var ppe_num = parseInt(stat.PPE_NUM);
					if (!isNaN(ppe_num)) {
						for (var i = 0; i < ppe_num; i++) {
							var ppe_bar = document.getElementById('ppe' + i + '_entry');
							if (ppe_bar)
								ppe_bar.innerHTML = E('td', {},
									progressbar(stat['BIND_PPE' + i], stat['ALL_PPE' + i])).innerHTML;
						}
					}

					var conntrack_bar = document.getElementById('conntrack_state');
					if (conntrack_bar)
						conntrack_bar.innerHTML = E('em', {},
							progressbar(parseInt(stat.Conntrack_Count), parseInt(stat.Conntrack_Max))
						).innerHTML;

					var cpu_bar = document.getElementById('cpu_usage_state');
					if (cpu_bar)
						cpu_bar.innerHTML = E('em', {}, formatCPUUsage(stat)).innerHTML;
				});
			}, 2);

			/* ---- initial render helpers ---- */
			function ppe_bar_html(stats, idx) {
				return progressbar(stats['BIND_PPE' + idx], stats['ALL_PPE' + idx]);
			}
			function ct_bar_html(stats) {
				if (!stats.Conntrack_Count) return E('em', {}, _('Collecting data...'));
				return progressbar(parseInt(stats.Conntrack_Count), parseInt(stats.Conntrack_Max));
			}
			function cpu_html(stats) {
				if (!stats.CPU_USED) return E('em', {}, _('Collecting data...'));
				return formatCPUUsage(stats);
			}
			var hasPPE = ppe_stats.hasOwnProperty('PPE_NUM');

			/* ---- build table rows ---- */
			var acc_rows = [
				E('tr', {}, [
					E('td', { 'width': '33%' }, _('FastPath Engine')),
					E('td', { 'id': 'fastpath_state' },
						svc_stats[0] && svc_stats[0].type
							? renderStatus(svc_stats)[0]
							: E('em', {}, _('Collecting data...')))
				]),
				E('tr', {}, [
					E('td', { 'width': '33%' }, _('Full Cone NAT')),
					E('td', { 'id': 'fullcone_state' },
						svc_stats[1] && svc_stats[1].type
							? renderStatus(svc_stats)[1]
							: E('em', {}, _('Collecting data...')))
				]),
				E('tr', {}, [
					E('td', { 'width': '33%' }, _('TCP CCA')),
					E('td', { 'id': 'tcpcca_state' },
						svc_stats[2] && svc_stats[2].type
							? renderStatus(svc_stats)[2]
							: E('em', {}, _('Collecting data...')))
				]),
				E('tr', {}, [
					E('td', { 'width': '33%' }, _('Conntrack')),
					E('td', { 'id': 'conntrack_state' }, ct_bar_html(ppe_stats))
				]),
				E('tr', {}, [
					E('td', { 'width': '33%' }, _('CPU Usage')),
					E('td', { 'id': 'cpu_usage_state' }, cpu_html(ppe_stats))
				])
			];

			if (hasPPE) {
				var ppe_num = parseInt(ppe_stats.PPE_NUM);
				for (var i = 0; i < ppe_num; i++) {
					acc_rows.push(E('tr', {}, [
						E('td', { 'width': '33%' }, 'PPE' + i + ' ' + _('Bind Entrys')),
						E('td', { 'id': 'ppe' + i + '_entry' }, ppe_bar_html(ppe_stats, i))
					]));
				}
			}

			return E('div', { 'class': 'cbi-section' }, [
				E('div', { 'class': 'cbi-section-title' }, _('Acceleration Status')),
				E('table', { 'class': 'table', 'width': '100%', 'cellspacing': '10' }, acc_rows)
			]);
		}

		/* Mark user edited (hidden section) */
		s = m.section(form.NamedSection, 'global', 'turboacc');
		s.render = function () { return E([]); };
		o = s.option(form.HiddenValue, 'set');
		o.load = (/* ... */) => { return 1 };
		o.readonly = true;
		o.rmempty = false;

		s = m.section(form.NamedSection, 'config', 'turboacc');

		o = s.option(form.ListValue, 'fastpath', _('Fastpath engine'),
			_('The offloading engine for routing/NAT.'));
		o.value('disabled', _('Disable'));
		if (features.hasFLOWOFFLOADING)
			o.value('flow_offloading', _('Flow offloading'));
		if (features.hasFASTCLASSIFIER)
			o.value('fast_classifier', _('Fast classifier'));
		if (features.hasSHORTCUTFECM)
			o.value('shortcut_fe_cm', _('SFE connection manager'));
		if (features.hasMEDIATEKHNAT)
			o.value('mediatek_hnat', _('MediaTek HNAT'));
		o.default = 'disabled';
		o.rmempty = false;
		o.onchange = function(ev, section_id, value) {
			var desc = ev.target.nextElementSibling;
			if (value === 'flow_offloading')
				desc.innerHTML = _('Software based offloading for routing/NAT.');
			else if (value === 'fast_classifier')
				desc.innerHTML = _('Fast classifier connection manager for the shortcut forwarding engine.');
			else if (value === 'shortcut_fe_cm')
				desc.innerHTML = _('Simple connection manager for the shortcut forwarding engine.');
			else if (value === 'mediatek_hnat')
				desc.innerHTML = _('MediaTek\'s open source hardware offloading engine.');
			else
				desc.innerHTML = _('The offloading engine for routing/NAT.');
		}

		o = s.option(form.Flag, 'fastpath_fo_hw', _('Hardware flow offloading'),
			_('Requires hardware NAT support. Implemented at least for mt7621.'));
		o.default = o.disabled;
		o.rmempty = false;
		o.depends('fastpath', 'flow_offloading');

		o = s.option(form.Flag, 'fastpath_fc_br', _('Bridge Acceleration'),
			_('Enable bridge acceleration (may be functional conflict with bridge-mode VPN server).'));
		o.default = o.disabled;
		o.rmempty = false;
		o.depends('fastpath', 'fast_classifier');

		if (features.hasIPV6) {
			o = s.option(form.Flag, 'fastpath_fc_ipv6', _('IPv6 acceleration'),
				_('Enable IPv6 Acceleration.'));
			o.default = o.disabled;
			o.rmempty = false;
			o.depends('fastpath', 'fast_classifier');
		}

		o = s.option(form.Flag, 'fastpath_mh_eth_hnat', _('Enable ethernet HNAT'),
			_('Enable hardware offloading for wired connections.'));
		o.default = o.enabled;
		o.rmempty = false;
		o.depends('fastpath', 'mediatek_hnat');

		o = s.option(form.Flag, 'fastpath_mh_eth_hnat_v6', _('Enable ethernet IPv6 HNAT'),
			_('Enable hardware offloading for wired IPv6 connections.'));
		o.default = o.enabled;
		o.rmempty = false;
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_bind_rate', _('HNAT bind rate threshold (pps)'),
			_('The smaller the threshold, the easier it is for the connection to be accelerated.'));
		o.optional = true;
		o.datatype = 'range(1,30)';
		o.placeholder = 30;
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Flag, 'fastpath_mh_eth_hnat_macvlan', _('Enable ethernet macvlan HNAT'),
			_('Also offload connections over macvlan interfaces (hnat_setting type 12).'));
		o.default = o.disabled;
		o.rmempty = false;
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Flag, 'fastpath_mh_eth_hnat_nf_stat', _('Sync HNAT counters to conntrack'),
			_('Let the PPE push flow counters back into nf_conntrack (hnat_setting type 7). Keep it on if accounting or conntrack statistics are in use.'));
		o.default = o.enabled;
		o.rmempty = false;
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_bind_threshold', _('HNAT binding threshold'),
			_('hnat_setting type 1, 0~65535. Empty means leave the driver default alone.'));
		o.optional = true;
		o.datatype = 'range(0,65535)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_tcp_life', _('TCP bind lifetime'),
			_('hnat_setting type 2, 0~65535. Age out bound TCP flows.'));
		o.optional = true;
		o.datatype = 'range(0,65535)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_fin_life', _('FIN bind lifetime'),
			_('hnat_setting type 3, 0~65535.'));
		o.optional = true;
		o.datatype = 'range(0,65535)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_udp_life', _('UDP bind lifetime'),
			_('hnat_setting type 4, 0~65535. Shorten it when QUIC/UDP floods the flow table.'));
		o.optional = true;
		o.datatype = 'range(0,65535)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_tcp_ka', _('TCP keep alive interval'),
			_('hnat_setting type 5, 0~255.'));
		o.optional = true;
		o.datatype = 'range(0,255)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_udp_ka', _('UDP keep alive interval'),
			_('hnat_setting type 6, 0~255.'));
		o.optional = true;
		o.datatype = 'range(0,255)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.ListValue, 'fastpath_mh_eth_hnat_ppenum', _('Number of HNAT PPE'),
			_('Appended to the mtkhnat line in /etc/modules.d as <code>ppe_cnt</code>, which kmodloader reads when loading the module at the next boot. Keep 2 unless the SoC really exposes a single PPE.'));
		o.rmempty = false;
		o.value(1);
		o.value(2);
		o.default = 2;
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.ListValue, 'fullcone', _('Full cone NAT'),
			_('Full cone NAT (NAT1) can improve gaming performance effectively.'));
		o.value('0', _('Disable'))
		o.value('2', _('Boardcom_FULLCONE_NAT'));
		o.default = '0';
		o.rmempty = false;

		o = s.option(form.ListValue, 'tcpcca', _('TCP CCA'),
			_('TCP congestion control algorithm.'));
		for (var i of features.hasTCPCCA.split(' ').sort())
			o.value(i);
		o.default = 'cubic';
		o.rmempty = false;

		return m.render();
	}
});
