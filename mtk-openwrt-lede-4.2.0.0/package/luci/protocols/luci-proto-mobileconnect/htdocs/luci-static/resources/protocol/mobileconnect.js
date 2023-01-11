'use strict';
'require rpc';
'require form';
'require network';

var callFileList = rpc.declare({
	object: 'file',
	method: 'list',
	params: [ 'path' ],
	expect: { entries: [] },
	filter: function(list, params) {
		var rv = [];
		for (var i = 0; i < list.length; i++)
			if (list[i].name.match(/^((usb0)|(wwan0)|(rmnet))/))
			    rv.push(params.path + list[i].name);
		return rv.sort();
	}
});


return network.registerProtocol('mobileconnect', {
	getI18n: function() {
		return _('Mobile_Connect');
	},

	getIfname: function() {
		return this._ubus('l3_device') || 'usb0' || "wwan0";
	},

	getOpkgPackage: function() {
		return 'quectel';
	},

	isFloating: function() {
		return true;
	},

	isVirtual: function() {
		return true;
	},

	getDevices: function() {
		return null;
	},

	containsDevice: function(ifname) {
		return (network.getIfnameOf(ifname) == this.getIfname());
	},

	renderFormOptions: function(s) {
		//var dev = this.getL3Device() || this.getDevice(), o;

		//o = s.taboption('general', form.Value, 'device', _('Modem device'));
		//o.rmempty = false;
		//o.load = function(section_id) {
		//	return callFileList('/sys/class/net/').then(L.bind(function(devices) {
               
		//		for (var i = 0; i < devices.length; i++) {
        //            var dl = devices[i].split('/')
        //            console.log(dl[dl.length-1])
		//			this.value(dl[dl.length-1]);
        //        }
		//		return form.Value.prototype.load.apply(this, [section_id]);
		//	}, this));
		//};

		s.taboption('general', form.Value, 'apn', _('APN'));
		s.taboption('general', form.Value, 'pincode', _('PIN'));
		s.taboption('general', form.Value, 'user', _('USER'));
		s.taboption('general', form.Value, 'passwd', _('PWD'));
	}
});
