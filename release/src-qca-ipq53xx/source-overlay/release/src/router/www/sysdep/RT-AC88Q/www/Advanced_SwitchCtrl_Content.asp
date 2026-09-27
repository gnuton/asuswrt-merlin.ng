<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<html xmlns:v>
<head>
<meta http-equiv="X-UA-Compatible" content="IE=Edge"/>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<link rel="shortcut icon" href="images/favicon.png">
<link rel="icon" href="images/favicon.png">
<title><#Web_Title#> - <#Switch_itemname#></title>
<link rel="stylesheet" type="text/css" href="index_style.css"> 
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="other.css">
<script type="text/javaScript" src="js/jquery.js"></script>
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/help.js"></script>
<script type="text/javascript" src="/detect.js"></script>
<script type="text/javascript" src="/validator.js"></script>
<style type="text/css">
.lan_trunk_icon {
	height: 60px;
	width: 60px;
	background-repeat: no-repeat;
	cursor: pointer;
}
.icon_enable_on {
	background-image: url('/images/New_ui/lan_port_enable_on.svg');
}
.icon_enable_off {
	background-image: url('/images/New_ui/lan_port_enable_off.svg');
}
.icon_handstand {
	transform: rotate(180deg);
}
.icon_disable {
	background-image: url('/images/New_ui/lan_port_disable.svg');
	cursor: default;
}
.lan_trunk_text {
	width: 60px;
	text-align: center;
}
.lan_trunk_text_disable {
	color: #888d8e;
}
.lan_port_hidden {
	width: 0px;
	height: 0px;
	opacity: 0;
}
.bondingHintBox {
	height: 160px;
	width: 210px;
	float: left;
}
.bondingHintBox:before{
	content: '';
	display: inline-block;
	vertical-align: middle;
	width: 0;
	height: 100%;
}
.bondingHintText{
	display: inline-block;
	vertical-align: middle;
	color: #FC0;
}
</style>
<script>
var wireless = [<% wl_auth_list(); %>];	// [[MAC, associated, authorized], ...]
var orig_lan_trunk_type = '<% nvram_get("lan_trunk_type"); %>';
var vlan_enable = '<% nvram_get("vlan_enable"); %>';

function initial(){
	show_menu();
	var nataccel = '<% nvram_get("qca_sfe"); %>';
	var nataccel_status = '<% nat_accel_status(); %>';

	if(nataccel == '1' && nataccel_status == '1'){
		document.getElementById("natAccelDesc").innerHTML = "<#NAT_Acceleration_enable#>";
	}
	else{
		document.getElementById("natAccelDesc").innerHTML = "<#NAT_Acceleration_ctf_disable#>";
	}
	if(productid == "BRT-AC828" || productid == "RT-AD7200") {
		var wans_dualwan = '<% nvram_get("wans_dualwan"); %>';
		var lan_flag = (wans_dualwan_orig.search("lan") == -1) ? false : true;
		var wans_dualwan_array = '<% nvram_get("wans_dualwan"); %>'.split(" ");

		if (wans_dualwan_array.indexOf("lan") != -1) {
			// LAN as WAN enabled, if both WAN1/WAN2 are not act as WAN, use them to do LAN LACP
			document.getElementById("tr_lan_trunk_type").style.display = "";
			document.getElementById("lan0_trunk").style.display = "none";
			document.getElementById("lan1_trunk").style.display = "none";
			document.getElementById("lan_trunk_hint").innerHTML = "<#NAT_lacp_note#>".replace(/LAN([1-2])/g, "WAN$1");

			val = [ 1, 2, 4 ];
			desc = [ "Layer2", "Layer2+3", "Layer3+4" ];
			cur = '<% nvram_get("lan_hash_algorithm"); %>';
			if (val.indexOf(parseInt(cur)) == -1)
				cur = val[desc.indexOf("Layer3+4")];
			add_options_x2(document.form.lan_hash_algorithm, desc, val, cur);

			val = [ 0, 2 ];
			desc = [ "<#btn_disable#>", "802.3ad" ];
			cur = '<% nvram_get("lan_trunk_type"); %>';
			if (val.indexOf(parseInt(cur)) == -1)
				cur = val[0];
			add_options_x2(document.form.lan_trunk_type_item, desc, val, cur);
		} else {
			// LAN as WAN disabled, use two of LAN1~LAN4 or LAN5~LAN8 port to trunk
			if(lan_flag) {
				document.form.lan_trunk_type_item.style.display = "none";
				document.form.lan_trunk_type_item.value = 0;
				document.form.lan_trunk_0.value = 0;
				document.form.lan_trunk_1.value = 0;
			}

			document.getElementById("tr_lan_trunk_type").style.display = "";
			document.getElementById("lan0_trunk").style.display = "";
			document.getElementById("lan1_trunk").style.display = "";
			changeTrunkType(document.form.lan_trunk_type_item);

			var gen_lan_trunk_icon = function(_group_id) {
				var html = "";
				var gen_lan_port = function(_port_idx, _group_id) {
					var lan_port_html = "";
					if(_port_idx % 2 == 1) {
						lan_port_html += "<div class='lan_text_" + _group_id + "_" + _port_idx + " lan_trunk_text'>LAN " + _port_idx +"</div>";
						lan_port_html += "<div id='lan" + _group_id + "_trunk_" + _port_idx + "_icon' class='lan_icon_" + _group_id + "_" + _port_idx + " lan_trunk_icon icon_handstand'></div>";
					}
					else {
						lan_port_html += "<div id='lan" + _group_id + "_trunk_" + _port_idx + "_icon' class='lan_icon_" + _group_id + "_" + _port_idx + " lan_trunk_icon'></div>";
						lan_port_html += "<div class='lan_text_" + _group_id + "_" + _port_idx + " lan_trunk_text'>LAN " + _port_idx +"</div>";
					}
					return lan_port_html;
				};
				for(var i = 1; i <= 8; i += 1) {
					if(i % 2 == 1) {
						html += "<div style='float:left;margin-right:5px;'>";
					}
					html += gen_lan_port(i, _group_id);
					if(i % 2 == 0) {
						html += "</div>";
					}
				}
				var groupNumHint = "(1 & 2) , (3 & 4) , (1 & 4) , (2 & 3)";
				if(_group_id == 1)
					groupNumHint = "5 & 6 or 7 & 8 or 5 & 8 or 6 & 7";
				html += "<div class='bondingHintBox'><p class='bondingHintText'><#SwitchCtrl_Select_At_Least_Port#></p></div>";
				return html;
			};

			document.getElementById("lan0_trunk_icon").innerHTML = gen_lan_trunk_icon(0);
			document.getElementById("lan1_trunk_icon").innerHTML = gen_lan_trunk_icon(1);

			var set_cb_lan_trunk = function(_group_id) {
				var lan_trunk = 0;
				if(_group_id == 0)
					lan_trunk = $("input[name=lan_trunk_" + _group_id + "]").val();
				else
					lan_trunk = $("input[name=lan_trunk_" + _group_id + "]").val() >> 4;
				var start_idx = _group_id * 4 + 1;
				var end_idx = _group_id * 4 + 4;
				for(var i = start_idx; lan_trunk != 0 && i <= end_idx; i += 1) {
					if(lan_trunk & 1){
						var name = "lan" + _group_id + "_trunk_" + i;
						var Elements = document.getElementsByName(name);
						Elements[0].checked = true;
					}
					lan_trunk = lan_trunk >> 1;
				}
			};
			set_cb_lan_trunk(0);
			set_cb_lan_trunk(1);
			set_link_aggregation_event();
			if(document.form.lan_trunk_type_item.value == "2")
				retune8023adLanPort();
		}
	}
}

function controlLanPort(event) {
	var group_id = event.data.group_id;
	var port_idx = event.data.port_idx;
	var name = "lan" + group_id + "_trunk_" + port_idx;
	var name_icon = name + "_icon";
	if(document.getElementsByName(name)[0].checked) {
		document.getElementsByName(name)[0].checked = false;
		document.getElementById(name_icon).className = document.getElementById(name_icon).className.replace("icon_enable_on", "icon_enable_off");
	}
	else {
		document.getElementsByName(name)[0].checked = true;
		document.getElementById(name_icon).className = document.getElementById(name_icon).className.replace("icon_enable_off", "icon_enable_on");
	}
	if(document.form.lan_trunk_type_item.value == "2")
		retune8023adLanPort();
}
function applyRule(){
	if(valid_form()){
		showLoading();
		document.form.submit();
	}
}

function valid_form(){
	if(productid == "BRT-AC828" || productid == "RT-AD7200") {
		var lan_trunk;
		var lan_trunk_type = "0";
		var lan_group_count = 0;
		var lan_group1_count = 0;
		var wans_dualwan_array = '<% nvram_get("wans_dualwan"); %>'.split(" ");

		if (wans_dualwan_array.indexOf("lan") != -1) {
			document.form.lan_trunk_type.value = document.form.lan_trunk_type_item.value;
		} else {
			var set_trunk_lan = function(_group_id, _lan_trunk_type) {
				var _lan_trunk = 0;
				if(_lan_trunk_type == "0") {
					_lan_trunk = 0;
					return _lan_trunk;
				}
				else {
					var _port_start_idx = (_group_id == 0) ? 1 : 5;
					var _port_end_idx = (_group_id == 0) ? 4 : 8;
					var _count = 0;
					_lan_trunk = 0;
					for(_port_start_idx; _port_start_idx <= _port_end_idx; _port_start_idx += 1) {
						var name = "lan" + _group_id + "_trunk_" + _port_start_idx;
						var Elements = document.getElementsByName(name);
						if(Elements[0].checked) {
							_lan_trunk |= (1 << (_port_start_idx - 1));
							_count++;
						}
					}

					if(_count == 0) {
						_lan_trunk = 0;
						return _lan_trunk;
					}
					if(_count < 2 && _count > 0) {
						alert("<#SwitchCtrl_At_Least_2_Ports_Group#>");
						_lan_trunk = -1;
						return _lan_trunk;
					}
					if(_lan_trunk_type == "2") { // 802.3ad, 2 ports only
						if(_count > 2) {
							alert("<#SwitchCtrl_Only_2_Ports_Group#>");
							_lan_trunk = -1;
							return _lan_trunk;
						}
					}
					if(_group_id == 0)
						lan_group_count = _count;
					else
						lan_group1_count = _count;

					return _lan_trunk;
				}
			};

			lan_trunk_type = document.form.lan_trunk_type_item.value;
			lan_trunk = set_trunk_lan(0, lan_trunk_type);
			if(lan_trunk == -1)
				return false;
			document.form.lan_trunk_0.value = lan_trunk;

			lan_trunk = set_trunk_lan(1, lan_trunk_type);
			if(lan_trunk == -1)
				return false;
			document.form.lan_trunk_1.value = lan_trunk;

			if(lan_trunk_type != "0") {
				if(document.form.lan_trunk_0.value == "0" && document.form.lan_trunk_1.value == "0") {
					alert("<#SwitchCtrl_At_Least_Group_Alert#>");
					return false;
				}
			}
			if(lan_trunk_type == "1") {
				if(document.form.lan_trunk_0.value % 5 == 0 && document.form.lan_trunk_0.value != 0 && lan_group_count != 4) { //only horizontal port
					alert("<#SwitchCtrl_Combination0_Alert#>");
					return false;
				}
				if(document.form.lan_trunk_1.value % 5 == 0 && document.form.lan_trunk_1.value != 0 && lan_group1_count != 4) { //only horizontal port
					alert("<#SwitchCtrl_Combination1_Alert#>");
					return false;
				}
			}
			if(lan_trunk_type == "2") {
				if(document.form.lan_trunk_0.value != "0" && document.form.lan_trunk_1.value != "0") {
					alert("<#SwitchCtrl_Only_2_Ports_Only_One_Group#>");
					return false;
				}

				if(document.form.lan_trunk_0.value % 5 == 0 && document.form.lan_trunk_0.value != 0) { //only horizontal port
					alert("<#SwitchCtrl_Combination0_Alert#>");
					return false;
				}
				else if(document.form.lan_trunk_1.value % 5 == 0 && document.form.lan_trunk_1.value != 0) { //only horizontal port
					alert("<#SwitchCtrl_Combination1_Alert#>");
					return false;
				}
			}

			document.form.lan_trunk_type.value = lan_trunk_type;
		}
	}
	else {
		document.form.lan_trunk_0.disabled = true;
		document.form.lan_trunk_1.disabled = true;
		document.form.lan_trunk_type.disabled = true;
	}
	return true;
}

function changeTrunkType(obj) {
	var wans_dualwan_array = '<% nvram_get("wans_dualwan"); %>'.split(" ");
	if (wans_dualwan_array.indexOf("lan") != -1) {
		// LAN as WAN enabled, if both WAN1/WAN2 are not act as WAN, use them to do LAN LACP
		document.getElementById("lan_trunk_hint").innerHTML = "<#NAT_lacp_note#>".replace(/LAN([1-2])/g, "WAN$1");
	} else {
		// LAN as WAN disabled, use two of LAN1~LAN4 or LAN5~LAN8 port to trunk
		if(vlan_enable != "0" && (orig_lan_trunk_type == "0" && obj.value != "0")){
			if(!confirm("<#SwitchCtrl_Disable_VLAN_Confirm#>"))
				return false;
			else{
				document.form.vlan_enable.disabled = false;
				document.form.vlan_enable.value = "0";
			}
		}

		document.getElementById("lan0_trunk").style.display = "none";
		document.getElementById("lan1_trunk").style.display = "none";
		document.getElementById("lan_trunk_hint").innerHTML = "";
		if(document.form.lan_trunk_type_item.style.display == "none") {
			document.getElementById("lan_trunk_hint").innerHTML = "<#SwitchCtrl_Bonding_Disable_Hint#>";
			document.getElementById("lan_trunk_hint").style.marginLeft = "0px";
		}
		else {
			$(".cb_lan0_trunk").prop("checked", false);
			$(".cb_lan1_trunk").prop("checked", false);
			switch(obj.value) {
				case "1" :
					document.getElementById("lan_trunk_hint").innerHTML = "<#SwitchCtrl_2to4_Each_Ports#>";
					document.getElementById("lan0_trunk").style.display = "";
					document.getElementById("lan1_trunk").style.display = "";
					break;
				case "2" :
					document.getElementById("lan_trunk_hint").innerHTML = "<#SwitchCtrl_Only_2_Ports_Only_One_Group#>";
					document.getElementById("lan0_trunk").style.display = "";
					document.getElementById("lan1_trunk").style.display = "";
					break;
			}
			set_link_aggregation_event();
		}
	}
}
function set_link_aggregation_event() {
	$(".lan_trunk_text").removeClass("lan_trunk_text_disable");
	$(".lan_trunk_icon").removeClass("icon_disable");
	$(".lan_trunk_icon").removeClass("icon_enable_on");
	$(".lan_trunk_icon").removeClass("icon_enable_off");
	$(".lan_trunk_icon").unbind("click");

	var getClassStartsWith = function(t,n) {
		var r = $.grep(t.split(" "), function(t,r) {
			return 0 === t.indexOf(n);
		}).join();
		return r || !1;
	};

	for(var i = 0; i < $(".lan_trunk_icon").length; i += 1) {
		var className = $(".lan_trunk_icon:eq(" + i + ")").attr("class");
		var group_and_idx = getClassStartsWith(className, "lan_icon").replace("lan_icon_", "");
		var group_id = group_and_idx.split("_")[0];
		var port_idx = group_and_idx.split("_")[1];
		if( (group_id == 0 && port_idx < 5) || (group_id == 1 && port_idx > 4) ) {
			var name = "lan" + group_id + "_trunk_" + port_idx;
			if(document.getElementsByName(name)[0].checked)
				$(".lan_icon_" + group_and_idx + "").addClass("icon_enable_on");
			else
				$(".lan_icon_" + group_and_idx + "").addClass("icon_enable_off");

			$(".lan_icon_" + group_and_idx + "").click({"group_id" : group_id, "port_idx": port_idx}, controlLanPort);
		}
		else {
			$(".lan_text_" + group_and_idx + "").addClass("lan_trunk_text_disable");
			$(".lan_icon_" + group_and_idx + "").addClass("icon_disable");
		}
	}
}
function retune8023adLanPort() {
	var group0_click_count = $(".cb_lan0_trunk:checked").length;
	var group1_click_count = $(".cb_lan1_trunk:checked").length;

	var set_lan_trunk_icon_default = function(_group_id) {
		$("#lan" + _group_id + "_trunk_icon").children().find(".lan_trunk_text").addClass("lan_trunk_text_disable");
		$("#lan" + _group_id + "_trunk_icon").children().find(".lan_trunk_icon").removeClass("icon_enable_off");
		$("#lan" + _group_id + "_trunk_icon").children().find(".lan_trunk_icon").addClass("icon_disable");
		$("#lan" + _group_id + "_trunk_icon").children().find(".lan_trunk_icon").unbind("click");
	};
	var set_lan_port_default = function(_group_id, _port_idx) {
		$(".lan_text_" + _group_id + "_" + _port_idx + "").addClass("lan_trunk_text_disable");
		$(".lan_icon_" + _group_id + "_" + _port_idx + "").removeClass("icon_enable_off");
		$(".lan_icon_" + _group_id + "_" + _port_idx + "").addClass("icon_disable");
		$(".lan_icon_" + _group_id + "_" + _port_idx + "").unbind("click");
	};
	var set_lan_port_enable = function(_group_id, _port_idx) {
		$(".lan_text_" + _group_id + "_" + _port_idx + "").removeClass("lan_trunk_text_disable");
		$(".lan_icon_" + _group_id + "_" + _port_idx + "").addClass("icon_enable_off");
		$(".lan_icon_" + _group_id + "_" + _port_idx + "").removeClass("icon_disable");
		$(".lan_icon_" + _group_id + "_" + _port_idx + "").unbind("click");
		$(".lan_icon_" + _group_id + "_" + _port_idx + "").click({"group_id" : _group_id, "port_idx": _port_idx}, controlLanPort);
	};
	var control_check_two_port = function(_group_id) {
		var unCheckLanPort = $(".cb_lan" + _group_id + "_trunk:not(:checked)");
		for(var i = 0; i < unCheckLanPort.length; i += 1) {
			var port_idx = $(".cb_lan" + _group_id + "_trunk:not(:checked):eq(" + i + ")").attr("name").replace("lan" + _group_id + "_trunk_", "");//get unchecked port idx
			set_lan_port_default(_group_id, port_idx);
		}
	};
	var control_check_one_port = function(_group_id) {
		var checkLanPort = $(".cb_lan" + _group_id + "_trunk:checked").attr("name").replace("lan" + _group_id + "_trunk_", "");//get checked port idx
		var start_idx = _group_id * 4 + 1;
		var end_idx = _group_id * 4 + 4;
		for(var i = start_idx; i <= end_idx; i += 1) {
			if(checkLanPort == i) //filter check lan port
				continue;
			if(checkLanPort % 2 == i % 2) //horizontal lan port
				set_lan_port_default(_group_id, i);
			else //vertical lan port
				set_lan_port_enable(_group_id, i);
		}
	};

	if(group0_click_count != 0) {
		set_lan_trunk_icon_default(1);
		if(group0_click_count == 2)
			control_check_two_port(0);
		else
			control_check_one_port(0);
	}
	else if(group1_click_count != 0) {
		set_lan_trunk_icon_default(0);
		if(group1_click_count == 2)
			control_check_two_port(1);
		else
			control_check_one_port(1);
	}
	else
		set_link_aggregation_event();
}
</script>
</head>

<body onload="initial();" onunLoad="return unload_body();">
<div id="TopBanner"></div>
<div id="hiddenMask" class="popup_bg">
	<table cellpadding="5" cellspacing="0" id="dr_sweet_advise" class="dr_sweet_advise" align="center">
		<tr>
		<td>
			<div class="drword" id="drword" style="height:110px;"><#Main_alert_proceeding_desc4#> <#Main_alert_proceeding_desc1#>...
				<br/>
				<br/>
	    </div>
		  <div class="drImg"><img src="images/alertImg.png"></div>
			<div style="height:70px;"></div>
		</td>
		</tr>
	</table>
<!--[if lte IE 6.5]><iframe class="hackiframe"></iframe><![endif]-->
</div>

<div id="Loading" class="popup_bg"></div>

<iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>

<form method="post" name="form" id="ruleForm" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="productid" value="<% nvram_get("productid"); %>">
<input type="hidden" name="current_page" value="Advanced_SwitchCtrl_Content.asp">
<input type="hidden" name="next_page" value="">
<input type="hidden" name="group_id" value="">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="action_mode" value="apply_new">
<input type="hidden" name="action_script" value="restart_allnet">
<input type="hidden" name="action_wait" value="30">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="lan_trunk_0" value="<% nvram_get("lan_trunk_0"); %>">
<input type="hidden" name="lan_trunk_1" value="<% nvram_get("lan_trunk_1"); %>">
<input type="hidden" name="lan_trunk_type" value="<% nvram_get("lan_trunk_type"); %>">
<input type="hidden" name="vlan_enable" value="<% nvram_get("vlan_enable"); %>" disabled>

<table class="content" align="center" cellpadding="0" cellspacing="0">
  <tr>
	<td width="17">&nbsp;</td>
	
	<!--=====Beginning of Main Menu=====-->
	<td valign="top" width="202">
	  <div id="mainMenu"></div>
	  <div id="subMenu"></div>
	</td>
	
    <td valign="top">
	<div id="tabMenu" class="submenuBlock"></div>
		<!--===================================Beginning of Main Content===========================================-->
<table width="98%" border="0" align="left" cellpadding="0" cellspacing="0">
	<tr>
		<td align="left" valign="top">
  <table width="760px" border="0" cellpadding="5" cellspacing="0" class="FormTitle" id="FormTitle">
	<tbody>
	<tr>
		  <td bgcolor="#4D595D" valign="top"  >
		  <div>&nbsp;</div>
		  <div class="formfonttitle"><#menu5_2#> - <#Switch_itemname#></div>
      <div style="margin: 10px 0 10px 5px" class="splitLine"></div>
      <div class="formfontdesc"><#SwitchCtrl_desc#></div>
		  
		  <table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
		  	
			<tr>
					<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(6, 7);"><#RouterConfig_GWMulticast_unknownUni_itemname#></a></th>
        	<td><!-- valid_muliticast(); -->
          	<input type="text" maxlength="4" class="input_6_table" id="switch_ctrlrate_unknown_unicast" name="switch_ctrlrate_unknown_unicast" value="<% nvram_get("switch_ctrlrate_unknown_unicast"); %>" onkeypress="return validator.isNumber(this, event);" autocorrect="off" autocapitalize="off">
          </td>
			</tr>

				<tr>
					<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(6, 7);"><#RouterConfig_GWMulticast_unknownMul_itemname#></a></th>
        	<td>
          	<input type="text" maxlength="4" class="input_6_table" id="switch_ctrlrate_unknown_multicast" name="switch_ctrlrate_unknown_multicast" value="<% nvram_get("switch_ctrlrate_unknown_multicast"); %>" onkeypress="return validator.isNumber(this, event);" autocorrect="off" autocapitalize="off">
          </td>
				</tr>

				<tr>
					<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(6, 7);"><#RouterConfig_GWMulticast_Multicast_itemname#></a></th>
        	<td>
          	<input type="text" maxlength="4" class="input_6_table" id="switch_ctrlrate_multicast" name="switch_ctrlrate_multicast" value="<% nvram_get("switch_ctrlrate_multicast"); %>" onkeypress="return validator.isNumber(this, event);" autocorrect="off" autocapitalize="off">
          </td>
				</tr>

				<tr>
					<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(6, 7);"><#RouterConfig_GWMulticast_Broadcast_itemname#></a></th>
        	<td>
          	<input type="text" maxlength="4" class="input_6_table" id="switch_ctrlrate_broadcast" name="switch_ctrlrate_broadcast" value="<% nvram_get("switch_ctrlrate_broadcast"); %>" onkeypress="return validator.isNumber(this, event);" autocorrect="off" autocapitalize="off">
          </td>
				</tr>
      	<tr>
		<th><#NAT_Acceleration#></th>
		<td>
			<select name="qca_sfe" class="input_option">
				<option class="content_input_fd" value="0" <% nvram_match("qca_sfe", "0","selected"); %>><#WLANConfig11b_WirelessCtrl_buttonname#></option>
				<option class="content_input_fd" value="1" <% nvram_match("qca_sfe", "1","selected"); %>><#Auto#></option>
			</select>
			&nbsp
			<span id="natAccelDesc"></span>
		</td>
      	</tr>					
	<tr>
		<th><#SwitchCtrl_Enable_GRO#></th>
		<td>
			<select name="qca_gro" class="input_option">
				<option class="content_input_fd" value="0" <% nvram_match("qca_gro", "0","selected"); %>><#WLANConfig11b_WirelessCtrl_buttonname#></option>
				<option class="content_input_fd" value="1" <% nvram_match("qca_gro", "1","selected"); %>><#Auto#></option>
			</select>
		</td>
	</tr>
	<tr>
		<th><#SwitchCtrl_Hash_Algo#></th>
		<td>
			<select name="lan_hash_algorithm" class="input_option">
				<option value="0" <% nvram_match( "lan_hash_algorithm", "0", "selected"); %>><#SwitchCtrl_Source_Port#></option>
				<option value="1" <% nvram_match( "lan_hash_algorithm", "1", "selected"); %>>Layer2</option><!--untranslated-->
				<option value="2" <% nvram_match( "lan_hash_algorithm", "2", "selected"); %>>Layer2+3</option><!--untranslated-->
				<option value="3" <% nvram_match( "lan_hash_algorithm", "3", "selected"); %>>Layer3</option><!--untranslated-->
				<option value="4" <% nvram_match( "lan_hash_algorithm", "4", "selected"); %>>Layer3+4</option><!--untranslated-->
			</select>
		</td>
	</tr>
	<tr id="tr_lan_trunk_type" style="display:none;">
		<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(29,1);"><#SwitchCtrl_Select_Bonding_Type#></a></th>
		<td>
			<select id="lan_trunk_type_item" name="lan_trunk_type_item" class="input_option" onchange="changeTrunkType(this);">
				<option value="0" <% nvram_match( "lan_trunk_type", "0", "selected"); %>><#btn_disable#></option>
				<option value="1" <% nvram_match( "lan_trunk_type", "1", "selected"); %>>Static</option><!--untranslated-->
				<option value="2" <% nvram_match( "lan_trunk_type", "2", "selected"); %>>802.3ad</option><!--untranslated-->
			</select>
			<span id="lan_trunk_hint" style="margin-left:5px;"></span>
		</td>
	</tr>
	<tr id="lan0_trunk" style="display:none;">
		<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(29,1);"><#SwitchCtrl_Select_Bonding_Group#> 0</a></th>
		<td>
			<div id="lan0_trunk_icon"></div>
			<div class='lan_port_hidden'>
				<input type="checkbox" name="lan0_trunk_1" class="input cb_lan0_trunk" value="1">LAN 1
				<input type="checkbox" name="lan0_trunk_2" class="input cb_lan0_trunk" value="2">LAN 2
				<input type="checkbox" name="lan0_trunk_3" class="input cb_lan0_trunk" value="4">LAN 3
				<input type="checkbox" name="lan0_trunk_4" class="input cb_lan0_trunk" value="8">LAN 4
			</div>
		</td>
	</tr>
	<tr id="lan1_trunk" style="display:none;">
		<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(29,1);"><#SwitchCtrl_Select_Bonding_Group#> 1</a></th>
		<td>
			<div id="lan1_trunk_icon"></div>
			<div class='lan_port_hidden'>
				<input type="checkbox" name="lan1_trunk_5" class="input cb_lan1_trunk" value="16">LAN 5
				<input type="checkbox" name="lan1_trunk_6" class="input cb_lan1_trunk" value="32">LAN 6
				<input type="checkbox" name="lan1_trunk_7" class="input cb_lan1_trunk" value="64">LAN 7
				<input type="checkbox" name="lan1_trunk_8" class="input cb_lan1_trunk" value="128">LAN 8
			</div>
		</td>
	</tr>
	</table>

		<div class="apply_gen">
			<input class="button_gen" onclick="applyRule()" type="button" value="<#CTL_apply#>"/>
		</div>
		
	  </td>
	</tr>

	</tbody>	
  </table>		
					
		</td>
	</form>					
				</tr>
			</table>				
			<!--===================================End of Main Content===========================================-->
</td>

    <td width="10" align="center" valign="top">&nbsp;</td>
	</tr>
</table>

<div id="footer"></div>
</body>
</html>
