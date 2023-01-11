'use strict';
'require view';
'require dom';
'require poll';
'require rpc';
'require uci';
'require form';
'require validation';

var callHostHints, callDUIDHints, CBISimCardStatus, callSimCardInfo;

callHostHints = rpc.declare({
	object: 'luci-rpc',
	method: 'getHostHints',
	expect: { '': {} }
});

callDUIDHints = rpc.declare({
	object: 'luci-rpc',
	method: 'getDUIDHints',
	expect: { '': {} }
});


callSimCardInfo = rpc.declare({
	object: 'luci-rpc',
	method: 'getSimCardInfo',
	expect: { '': {} }
});

CBISimCardStatus = form.DummyValue.extend({
	renderWidget: function(section_id, option_id, cfgvalue) {
		return E([
			E('h4', _('Sim Card Status')),
			E('div', { 'id': 'simcard_status_table', 'class': 'table' }, [
				E('div', { 'class': 'tr table-titles' }, [
					E('div', { 'class': 'th' }, _('SimStatus')),
					E('div', { 'class': 'th' }, _('IMEI')),
					E('div', { 'class': 'th' }, _('Network')),
					E('div', { 'class': 'th' }, _('RSSI')),
					E('div', { 'class': 'th' }, _('NetType'))
					//E('div', { 'class': 'th' }, _('NetStatus'))
				]),
				E('div', { 'class': 'tr placeholder' }, [
					E('div', { 'class': 'td' }, E('em', _('Collecting data...')))
				])
			])
		]);
	}
});

return view.extend({
	load: function() {
		return Promise.all([
			callHostHints(),
			callDUIDHints()
		]);
	},

	render: function(hosts_duids) {
		var hosts = hosts_duids[0],
		    duids = hosts_duids[1],
		    m, s, o, ss, so;

		m = new form.Map('mobile_app', _('HLK 5G'), _('HiLink Mobile App'));	///etc/config/hlk-5g

		s = m.section(form.TypedSection, 'mobile_app', _('MobileApp'));	///etc/config/hlk-5g config hlk-5g	->/etc/config/mobile_app
		s.anonymous = true;
		s.addremove = false;

		s.tab('status', _('Mobile Status'));

		o = s.taboption('status', CBISimCardStatus, '__sim_card_status__');

		return m.render().then(function(mapEl) {
			poll.add(function() {
				return callSimCardInfo().then(function(siminfo) {
					var simcard = Array.isArray(siminfo.simcard) ? siminfo.simcard : [];
					    //imei = Array.isArray(siminfo.imei) ? siminfo.imei : [];

					cbi_update_table(mapEl.querySelector('#simcard_status_table'),
                            simcard.map(function(simcard_info) {

							return [
								simcard_info.simstatus || 'ABSENT',
								simcard_info.imei || '-',
								simcard_info.network || '-',
								simcard_info.rssi || '-',
								simcard_info.nettype || '-',
							];
						}),
						E('em', _('There are no active Sim')));
				});
			});

			return mapEl;
		});
	}
});

