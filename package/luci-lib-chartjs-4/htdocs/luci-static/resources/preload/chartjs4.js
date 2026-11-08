'use strict';
'require baseclass';

/* Chart.js 4 只发布 UMD 包，morselinkeval 这类应用是直接引用全局 `Chart` 的，
 * 所以借助 luci-base 的 preload 目录把 bundle 挂进页面，省得每个应用自己去
 * createElement('script')。 */
return baseclass.extend({
	__init__: function() {
		if (typeof Chart !== 'undefined')
			return;

		var script = document.createElement('script');
		script.src = L.resource('chart.umd.min.js');
		script.async = false;
		document.head.appendChild(script);
	}
});
