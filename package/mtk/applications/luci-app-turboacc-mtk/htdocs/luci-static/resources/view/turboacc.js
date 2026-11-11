/* Copyright (C) 2022 ImmortalWrt.org */

'use strict';
'require form';
'require fs';
'require poll';
'require rpc';
'require uci';
'require view';

let callSystemFeatures = rpc.declare({
	object: 'luci.turboacc',
	method: 'getSystemFeatures',
	expect: { '': {} }
});

let callFastPathStat = rpc.declare({
	object: 'luci.turboacc',
	method: 'getFastPathStat',
	expect: { '': {} }
});

let callFullConeStat = rpc.declare({
	object: 'luci.turboacc',
	method: 'getFullConeStat',
	expect: { '': {} }
});

let callTCPCCAStat = rpc.declare({
	object: 'luci.turboacc',
	method: 'getTCPCCAStat',
	expect: { '': {} }
});

let callMTKPPEStat = rpc.declare({
	object: 'luci.turboacc',
	method: 'getMTKPPEStat',
	expect: { '': {} }
});

function renderProgressBar(value, max, byte) {
	let vn = parseInt(value) || 0,
		mn = parseInt(max) || 100,
		pc = Math.floor((100 / mn) * vn),
		text_val = byte ? String.format('%1024.2mB', value) : value,
		text_max = byte ? String.format('%1024.2mB', max) : max;

	return E('div', {
		'class': 'cbi-progressbar',
		'title': '%s / %s (%d%%)'.format(text_val, text_max, pc)
	}, E('div', { 'style': 'width:%.2f%%'.format(pc) }));
}

function renderStatusItem(stat) {
	if (!stat || !stat.type) {
		return E('em', { 'style': 'color:red; font-weight:bold' }, _('Disabled'));
	}

	return E('em', { 'style': 'color:green; font-weight:bold' }, stat.type);
}

function formatCPUUsage(stats) {
	let text = stats.CPU_USED || '0%';

	if (stats.CPU_CORES_USED) {
		let cores = [];
		for (let core in stats.CPU_CORES_USED)
			cores.push(core);

		cores.sort(function (a, b) {
			return parseInt(a.replace(/^CPU/i, '')) - parseInt(b.replace(/^CPU/i, ''));
		});

		let items = [];
		for (let i = 0; i < cores.length; i++)
			items.push(cores[i] + ': ' + stats.CPU_CORES_USED[cores[i]]);

		if (items.length > 0)
			text += ' (' + items.join(', ') + ')';
	}

	return text;
}

return view.extend({
	load: function() {
		return Promise.all([
			uci.load('turboacc'),
			L.resolveDefault(callSystemFeatures(), {}),
			L.resolveDefault(callMTKPPEStat(), {})
		]);
	},

	render: function(data) {
		let features = data[1];
		let ppe_stats = data[2];

		/* only check ppe when mtk_hnat is enabled */
		let current_fastpath = uci.get('turboacc', 'config', 'fastpath');
		let has_ppe = (current_fastpath == 'mediatek_hnat') && ppe_stats && ppe_stats['PPE num'];

		let m, s, o;

		m = new form.Map('turboacc', _('TurboACC settings'),
			_('Open source flow offloading engine (fast path or hardware NAT).'));

		s = m.section(form.TypedSection);
		s.anonymous = true;
		s.render = function () {
			/* Basic Status */
			let table_rows = [
				E('tr', {}, [
					E('td', { 'class': 'tdLeft', 'width': '33%' }, _('FastPath Engine')),
					E('td', { 'id': 'fastpath_state' }, E('em', {}, _('Collecting data...')))
				]),
				E('tr', {}, [
					E('td', { 'class': 'tdLeft' }, _('Full Cone NAT')),
					E('td', { 'id': 'fullcone_state' }, E('em', {}, _('Collecting data...')))
				]),
				E('tr', {}, [
					E('td', { 'class': 'tdLeft' }, _('TCP CCA')),
					E('td', { 'id': 'tcpcca_state' }, E('em', {}, _('Collecting data...')))
				]),
				E('tr', {}, [
					E('td', { 'class': 'tdLeft' }, _('Conntrack')),
					E('td', { 'id': 'conntrack_state' }, E('em', {}, _('Collecting data...')))
				]),
				E('tr', {}, [
					E('td', { 'class': 'tdLeft' }, _('CPU Usage')),
					E('td', { 'id': 'cpu_usage_state' }, E('em', {}, _('Collecting data...')))
				])
			];

			/* PPE Stats */
			if (has_ppe) {
				let ppe_num = parseInt(ppe_stats['PPE num']);
				for (let i = 0; i < ppe_num; i++) {
					let ppe_key = 'PPE' + i;
					if (ppe_stats[ppe_key]) {
						table_rows.push(E('tr', {}, [
							E('td', { 'class': 'tdLeft' }, `${ppe_key} ` + _('Bind Entries')),
							E('td', { 'id': `ppe${i}_entry` }, E('em', {}, _('Collecting data...')))
						]));
					}
				}
			}

			let acc_status = E('table', { 'class': 'table' }, table_rows);

			poll.add(async function () {
				let res = await Promise.all([
					L.resolveDefault(callFastPathStat(), {}),
					L.resolveDefault(callFullConeStat(), {}),
					L.resolveDefault(callTCPCCAStat(), {}),
					L.resolveDefault(callMTKPPEStat(), {})
				]);

				const dom_ids = ['fastpath_state', 'fullcone_state', 'tcpcca_state'];
				for (let i = 0; i < 3; i++) {
					let el = document.getElementById(dom_ids[i]);
					if (el) {
						L.dom.content(el, renderStatusItem(res[i]));
					}
				}

				let stat = res[3] || {};

				let ct = document.getElementById('conntrack_state');
				if (ct) {
					L.dom.content(ct, renderProgressBar(parseInt(stat.Conntrack_Count), parseInt(stat.Conntrack_Max)));
				}

				let cpu = document.getElementById('cpu_usage_state');
				if (cpu) {
					L.dom.content(cpu, E('em', {}, formatCPUUsage(stat)));
				}

				if (has_ppe) {
					let ppe_num = parseInt(stat['PPE num'] || 0);
					for (let j = 0; j < ppe_num; j++) {
						let ppe_key = 'PPE' + j;
						let ppe_bar = document.getElementById(`ppe${j}_entry`);

						if (ppe_bar && stat[ppe_key]) {
							let bind_num = stat[ppe_key]['BIND state num'];
							let total_num = stat[ppe_key]['entry num'];
							L.dom.content(ppe_bar, renderProgressBar(bind_num, total_num));
						}
					}
				}
			});

			return E('div', { 'class': 'cbi-section' }, [
				E('h3', {}, _('Acceleration Status')),
				acc_status
			]);
		};

		s = m.section(form.NamedSection, 'config', 'turboacc');

		o = s.option(form.ListValue, 'fastpath', _('Fastpath engine'),
			_('The offloading engine for routing/NAT.'));
		o.value('disabled', _('Disable'));

		if (features.hasFLOWOFFLOADING) o.value('flow_offloading', _('Flow offloading'));
		if (features.hasFASTCLASSIFIER) o.value('fast_classifier', _('Fast classifier'));
		if (features.hasSHORTCUTFECM) o.value('shortcut_fe_cm', _('SFE connection manager'));
		if (features.hasMEDIATEKHNAT) o.value('mediatek_hnat', _('MediaTek HNAT'));

		o.default = 'disabled';

		const descMap = {
			'flow_offloading': _('Software based offloading for routing/NAT.'),
			'fast_classifier': _('Fast classifier connection manager for the shortcut forwarding engine.'),
			'shortcut_fe_cm': _('Simple connection manager for the shortcut forwarding engine.'),
			'mediatek_hnat': _('MediaTek\'s open source hardware offloading engine.'),
			'default': _('The offloading engine for routing/NAT.')
		};

		o.onchange = function(ev, section_id, value) {
			let desc = descMap[value] || descMap['default'];
			let el = this.getUIElement(section_id);
			if (el && el.node && el.node.parentNode) {
				let descEl = el.node.parentNode.querySelector('.cbi-value-description');
				if (descEl) descEl.innerHTML = desc;
			}
		};

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

		o = s.option(form.Value, 'fastpath_mh_bind_rate', _('HNAT bind rate threshold (pps)'),
			_('hnat_setting type 1 (PPE_BNDR/BIND_RATE), 0~65535. The smaller the threshold, the easier it is for the connection to be accelerated. This tree exposes a single binding-threshold knob; the separate 24.10 pps limiter (type 11) does not exist here.'));
		o.optional = true;
		o.datatype = 'range(0,65535)';
		o.placeholder = 30;
		o.default = 30;
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Flag, 'fastpath_mh_update_nfct', _('Enable HNAT counter update'),
			_('Update HNAT counter to nf_conntrack. May impact performance. (hnat_setting type 7)'));
		o.default = o.disabled;
		o.rmempty = false;
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_tcp_life', _('TCP bind lifetime'),
			_('hnat_setting type 2, 0~65535. Age out bound TCP flows. Empty leaves the driver default.'));
		o.optional = true;
		o.datatype = 'range(0,65535)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_fin_life', _('FIN bind lifetime'),
			_('hnat_setting type 3, 0~65535. Empty leaves the driver default.'));
		o.optional = true;
		o.datatype = 'range(0,65535)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_udp_life', _('UDP bind lifetime'),
			_('hnat_setting type 4, 0~65535. Shorten it when QUIC/UDP floods the flow table. Empty leaves the driver default.'));
		o.optional = true;
		o.datatype = 'range(0,65535)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_tcp_ka', _('TCP keep alive interval'),
			_('hnat_setting type 5, 0~255. Empty leaves the driver default.'));
		o.optional = true;
		o.datatype = 'range(0,255)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.Value, 'fastpath_mh_eth_hnat_udp_ka', _('UDP keep alive interval'),
			_('hnat_setting type 6, 0~255. Empty leaves the driver default.'));
		o.optional = true;
		o.datatype = 'range(0,255)';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.ListValue, 'fastpath_mh_eth_hnat_ppenum', _('Number of HNAT PPE'),
			_('Appended to the mtkhnat line in /etc/modules.d as ppe_cnt, which kmodloader reads when loading the module at the next boot. Keep 2 unless the SoC really exposes a single PPE.'));
		o.rmempty = false;
		o.value('1', '1');
		o.value('2', '2');
		o.default = '2';
		o.depends('fastpath_mh_eth_hnat', '1');

		o = s.option(form.ListValue, 'tcpcca', _('TCP CCA'),
			_('TCP congestion control algorithm.'));
		
		if (features.hasTCPCCA) {
			let algos = features.hasTCPCCA.split(' ').sort();
			for (let i = 0; i < algos.length; i++) {
				o.value(algos[i]);
			}
		}
		o.default = 'bbr';
		o.rmempty = false;

		return m.render();
	}
});
