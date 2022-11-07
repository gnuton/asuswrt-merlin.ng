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
<title><#Web_Title#> - VDSL</title>
<link rel="stylesheet" type="text/css" href="index_style.css">
<link rel="stylesheet" type="text/css" href="form_style.css">
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/help.js"></script>
<script type="text/javascript" src="/validator.js"></script>
<script type="text/javascript" src="/js/jquery.js"></script>
<style>
.FormTable{
	margin-top:10px;
}
</style>
<script>

<% login_state_hook(); %>
<% dsl_get_parameter(); %>

var dsl_pvc_enabled = ["0", "0", "0", "0", "0", "0", "0", "0"];

var wans_dualwan = '<% nvram_get("wans_dualwan"); %>';
<% wan_get_parameter(); %>
var original_dnsenable = parseInt('<% nvram_get("dsl_dnsenable"); %>');
var original_ppp_echo = parseInt('<% nvram_get("wan_ppp_echo"); %>');
var default_ppp_echo = parseInt('<% nvram_default_get("wan_ppp_echo"); %>');
var orig_mtu = '<% nvram_get("dsl_mtu"); %>';

var chg_pvc_unit_flag = '<% get_parameter("chg_pvc"); %>';

var load_wan_unit = '<% nvram_get("wan_unit"); %>';
if(load_wan_unit.length == 3)
	load_wan_unit=load_wan_unit.substring(1, 2);

if(dnspriv_support){
	//var dot_servers_array = [];
	var dnspriv_rulelist_array = '<% nvram_get("dnspriv_rulelist"); %>';
}

if(dualWAN_support){
	var wan_type_name = wans_dualwan.split(" ")[load_wan_unit];
	wan_type_name = wan_type_name.toUpperCase();
	switch(wan_type_name){
		case "WAN":
			location.href = "Advanced_WAN_Content.asp";
			break;
		case "LAN":
			location.href = "Advanced_WAN_Content.asp";
			break;
		case "USB":
			location.href = "Advanced_Modem_Content.asp";
			break;
		default:
			break;
	}
}

function Check_IE_Version(){ //Edge: return 12
    var rv = -1; // Return value assumes failure.

    if (navigator.appName == 'Microsoft Internet Explorer'){

       var ua = navigator.userAgent,
           re  = new RegExp("MSIE ([0-9]{1,}[\\.0-9]{0,})");

       if (re.exec(ua) !== null){
         rv = parseFloat( RegExp.$1 );
       }
    }
    else if(navigator.appName == "Netscape"){                       
       /// in IE 11 the navigator.appVersion says 'trident'
       /// in Edge the navigator.appVersion does not say trident
       if(navigator.appVersion.indexOf('Trident') === -1 && navigator.appVersion.indexOf('Edge') >= 0) rv = 12;
       else rv = 11;
    }       

    return rv;          
}
var IE_Version = Check_IE_Version();

// get PTM WAN setting list from hook.
// dsl_enable, dsl_proto, dsl_dot1q, dsl_vid
var DSLWANList = [ <% get_DSL_WAN_list(); %> ];

function chg_pvc_unit(pvc_to_chg) {
	dr_advise();
	document.getElementById("drword").innerHTML = "&nbsp;&nbsp;&nbsp;<#QIS_autoMAC_desc2#>...";
	document.chgpvc.dsl_subunit.value=pvc_to_chg.toString();
	document.chgpvc.submit();
}

function chg_pvc(pvc_to_chg) {

	disable_pvc_summary();
	enable_all_ctrl(pvc_to_chg);
	document.form.dsl_subunit.value = pvc_to_chg;
	document.form.add_pvc_flag.value = "0";

	if (pvc_to_chg != "0") {
		if(!mswan_support){
			remove_item_from_select_bridge();
		}
		else{
			renew_dsl_proto_options();
		}
	}
	else
	{
		iptv_row = 0;
		remove_bridge();	//remove beidge while edit Internet PVC
	}
	
	change_dsl_unit_idx(8,pvc_to_chg);
	change_dsl_type(document.form.dsl_proto.value);
	fixed_change_dsl_type(document.form.dsl_proto.value);

	if (pvc_to_chg != "0") {	//useless setup
		inputCtrl(document.form.dsl_upnp_enable[0], 0);
		inputCtrl(document.form.dsl_upnp_enable[1], 0);
	}
}

function remove_item_from_select_bridge() {
	free_options(document.form.dsl_proto);
	var var_item = new Option("Bridge", "bridge");
	document.form.dsl_proto.options.add(var_item);
}

function renew_dsl_proto_options(){
	free_options(document.form.dsl_proto);			//remove beidge while edit Internet PVC if not mswan_support
	var var_item0 = new Option("<#BOP_ctype_title1#>", "dhcp");
	var var_item1 = new Option("<#BOP_ctype_title5#>", "static");
	var var_item2 = new Option("PPPoE", "pppoe");
	var var_item3 = new Option("Bridge", "bridge");
	document.form.dsl_proto.options.add(var_item0);
	document.form.dsl_proto.options.add(var_item1);
	document.form.dsl_proto.options.add(var_item2);
	document.form.dsl_proto.options.add(var_item3);
}

function remove_bridge(){
	free_options(document.form.dsl_proto);			//remove beidge while edit Internet PVC if not mswan_support
	var var_item0 = new Option("<#BOP_ctype_title1#>", "dhcp");
	var var_item1 = new Option("<#BOP_ctype_title5#>", "static");
	var var_item2 = new Option("PPPoE", "pppoe");
	document.form.dsl_proto.options.add(var_item0);
	document.form.dsl_proto.options.add(var_item1);
	document.form.dsl_proto.options.add(var_item2);
}

function add_pvc(){

	// find a available PVC
	var avail_pvc = 7;
	var found_pvc = false;
	for(var j = 1; j < DSLWANList.length; j++){
			if (DSLWANList[j][0] == "0") {
				found_pvc = true;
				avail_pvc = j;
				break;
			}
	}
	if (found_pvc == false) {
			// no empty pvc , return
			return;
	}

	document.form.dsl_subunit.disabled = false;
	document.form.dsl_subunit.value = avail_pvc.toString();	//sub-unit:1~7, 	0 for null
	document.form.add_pvc_flag.value = "1";

	enable_all_ctrl(document.form.dsl_subunit.value);

	if(avail_pvc == "0"){
		document.getElementById("pvc_sel").innerHTML = "Internet PVC";
	}
	else{
		document.getElementById("pvc_sel").innerHTML = "IPTV PVC #"+avail_pvc.toString();
	}

	if(!mswan_support){
		remove_item_from_select_bridge();
	}
	else{
		renew_dsl_proto_options();
	}

	document.getElementById("dslSettings").style.display = "";

	if (document.form.dsl_proto.value != "bridge") {
		document.getElementById("IPsetting").style.display = "";
		document.getElementById("DNSsetting").style.display = "";
		document.getElementById("PPPsetting").style.display = "";
		document.getElementById("vpn_server").style.display = "";
	}
	else {
		document.getElementById("IPsetting").style.display = "none";
		document.getElementById("DNSsetting").style.display = "none";
		document.getElementById("PPPsetting").style.display = "none";
		document.getElementById("vpn_server").style.display = "none";
	}

	if(IE_Version == 12){	//for IE/Edge only
		setTimeout("disable_pvc_summary();",500);
	}
	else{
		disable_pvc_summary();
	}
	
	change_dsl_type(document.form.dsl_proto.value);
	fixed_change_dsl_type(document.form.dsl_proto.value);
	document.form.dsl_link_enable[0].checked = true;	//Add to enable it

	//useless setup
	inputCtrl(document.form.dsl_upnp_enable[0], 0);
	inputCtrl(document.form.dsl_upnp_enable[1], 0);
}

function add_pvc_0() {
	disable_pvc_summary();
	enable_all_ctrl(0);
	document.getElementById("pvc_sel").innerHTML = "Internet PVC";
	
	remove_bridge();

	document.form.dsl_subunit.value = 0;
	document.form.dsl_proto.value = ("<% nvram_get("dsl8_proto"); %>" == "")? "dhcp":"<% nvram_get("dsl8_proto"); %>";
	change_dsl_type(document.form.dsl_proto.value);
	fixed_change_dsl_type(document.form.dsl_proto.value);
	document.form.dsl_link_enable[0].checked = true;	//Add to enable it

}

function del_pvc_sub(pvc_to_del) {
	var msg = "";
	if(pvc_to_del == 0)
		msg += "Internet, ";
	else
		msg += "IPTV "+pvc_to_del.toString()+", ";

	if(DSLWANList[pvc_to_del][2] != 0)
		msg += "VLAN ID = "+DSLWANList[pvc_to_del][3].toString();
	else
		msg += "802.1Q Disable";
	msg += "\n\n";
	msg += "<#pvc_del_alert_desc#>";
	var answer = confirm (msg);
	if (answer == false)
			return;
	document.form.dsl_subunit.disabled = false;
	document.form.dsl_subunit.value = pvc_to_del.toString();
	document.form.dsl_enable.value = "0";
	del_pvc_submit();
}

function showDSLWANList(){
	if(isSupport("is_ax5400_i1")){
		document.getElementById('DSL_WAN_add_del').style.display = "none";
	}
	var addRow;
	var cell = new Array(8);
	var config_num = 0;
	for(var i = 0; i < DSLWANList.length; i++){
		if (DSLWANList[i][0] != "0") {	//enabled unit
			config_num++;
		}
	}
	if(config_num == 0){
		if(!isSupport("is_ax5400_i1")){
			addRow = document.getElementById('DSL_WAN_table').insertRow(2); //0:thead 1:th 2:the 1st rule
			cell[0] = addRow.insertCell(0);
			cell[0].colSpan = "8";
			cell[0].style.color = "white";
			cell[0].innerHTML = '<center><input class="add_btn" onclick="add_pvc_0();" value=""/></center>';
		}
	}
	else{
		var row_count=0;
		// dsl_enable, dsl_proto, dsl_dot1q, dsl_vid
		for(var i = 0; i < DSLWANList.length; i++){
			
			if (DSLWANList[i][0] != "0") {
				addRow = document.getElementById('DSL_WAN_table').insertRow(row_count+2);
				row_count++;
				cell[0] = addRow.insertCell(0);
				cell[0].innerHTML = "<center>"+i+"</center>";
				cell[0].style.color = "white";
				cell[1] = addRow.insertCell(1);
				if (DSLWANList[i][2]=="1") cell[1].innerHTML = "<center><#checkbox_Yes#></center>";
				else cell[1].innerHTML = "<center><#checkbox_No#></center>";
				cell[1].style.color = "white";
				cell[2] = addRow.insertCell(2);
				cell[2].innerHTML = "<center>"+DSLWANList[i][3]+"</center>";
				cell[2].style.color = "white";
				cell[3] = addRow.insertCell(3);
				if (DSLWANList[i][1]=="pppoe") cell[3].innerHTML = "<center>PPPoE</center>";
				else if (DSLWANList[i][1]=="dhcp") cell[3].innerHTML = "<center><#BOP_ctype_title1#></center>";
				else if (DSLWANList[i][1]=="bridge") cell[3].innerHTML = "<center>Bridge</center>";
				else if (DSLWANList[i][1]=="static") cell[3].innerHTML = "<center><#BOP_ctype_title5#></center>";
				else cell[3].innerHTML = "Unknown";
				cell[3].style.color = "white";

				cell[4] = addRow.insertCell(4);
				if (i==0) {
					cell[4].innerHTML = "<center><img src=images/checked.gif border=0></center>";
				}
				else{
					cell[4].innerHTML = "";
				}
				cell[4].style.color = "white";

				cell[5] = addRow.insertCell(5);
				if (i==0) {
					cell[5].innerHTML = '';
				}
				else{
					cell[5].innerHTML = "<center><img src=images/checked.gif border=0></center>";
				}
				cell[5].style.color = "white";

				cell[6] = addRow.insertCell(6);
				cell[6].innerHTML = '<center><span style="cursor:pointer;" onclick="chg_pvc_unit('+i.toString()+');"><input class="edit_btn"></span></center>';
				cell[6].style.color = "white";

				if(!isSupport("is_ax5400_i1")){
					cell[7] = addRow.insertCell(7);
					cell[7].innerHTML = '<center><input class="remove_btn" onclick="del_pvc_sub('+i.toString()+');" value=""/></center>';
					cell[7].style.color = "white";
				}
			}
		}

		if (row_count < 8) {
			if(!isSupport("is_ax5400_i1")){
				addRow = document.getElementById('DSL_WAN_table').insertRow(row_count+2);
				cell[0] = addRow.insertCell(0);
				cell[0].colSpan = "8";
				cell[0].style.color = "white";
				if(DSLWANList[0][0] != "0"){
					cell[0].innerHTML = '<center><input class="add_btn" onclick="add_pvc();" value=""/></center>';
				}
				else{
					cell[0].innerHTML = '<center><input class="add_btn" onclick="add_pvc_0();" value=""/></center>';
				}
			}
		}
	}
}

function initial(){
	show_menu();

	document.form.dsl_dhcp_clientid.value = decodeURIComponent('<% nvram_char_to_ascii("", "dsl_dhcp_clientid"); %>');

	// WAN port
	genWANSoption();
	change_wan_unit(document.form.wan_unit);

	if(!dualWAN_support && !vdsl_support) {
		document.getElementById("WANscap").style.display = "none";
	}

	if(chg_pvc_unit_flag.length > 0 && chg_pvc_unit_flag >= 0 && chg_pvc_unit_flag <= 7){
		chg_pvc(chg_pvc_unit_flag);
	}
	else{
		showDSLWANList();
		disable_all_ctrl();
	}

	$.getJSON("https://nw-dlcdnet.asus.com/plugin/js/dns_db.json",
		function(data){
			var dns_db_translation_mapping = [
				{tag:"#ADGUARD_1",text:"<#IPConnection_x_DNS_DB_ADGUARD_1#>"},
				{tag:"#ADGUARD_2",text:"<#IPConnection_x_DNS_DB_ADGUARD_2#>"},
				{tag:"#CLOUDFLARE_1",text:"<#IPConnection_x_DNS_DB_CLOUDFLARE_1#>"},
				{tag:"#CLOUDFLARE_2",text:"<#IPConnection_x_DNS_DB_CLOUDFLARE_2#>"},
				{tag:"#CLOUDFLARE_3",text:"<#IPConnection_x_DNS_DB_CLOUDFLARE_3#>"}
			];
			Object.keys(data).forEach(function(dns_item) {
				var dns_name = data[dns_item].name;
				var dns_list = data[dns_item].server;
				var dns_desc = data[dns_item].desc;
				var dns_translation = data[dns_item].translation;
				Object.keys(dns_list).forEach(function(idx) {
					var dns_ip = dns_list[idx];
					var $dns_item_bg = $("<a>");
					$dns_item_bg.appendTo($("#dns_server_list1"));
					if(dns_desc != "")
						$dns_item_bg.attr("title", dns_desc);
					if(dns_translation != "") {
						var specific_translation = dns_db_translation_mapping.filter(function(item, index, _array){
							return (item.tag == dns_translation);
						})[0];
						if(specific_translation != undefined)
							$dns_item_bg.attr("title",  specific_translation.text);
					}
					var $dns_item = $("<div>");
					$dns_item.appendTo($dns_item_bg);
					$dns_item.unbind("click");
					$dns_item.click(function(e) {
						e = e || event;
						e.stopPropagation();
						var click_dns_ip = $(this).children("strong").attr("dns_ip");
						var idx = $(this).closest(".dns_server_list_dropdown").attr("id").replace("dns_server_list", "");
						$("input[name='dsl_dns" + idx + "']").val(click_dns_ip);
						$(".dns_pull_arrow").attr("src","/images/arrow-down.gif");
						$(".dns_server_list_dropdown").hide();
					});
					var $dns_text = $("<strong>");
					$dns_text.appendTo($dns_item);
					$dns_text.html("" + dns_name + " ( " + dns_ip +  " )");
					$dns_text.attr("dns_ip", dns_ip);
				});
			});
			$("#dns_server_list1").children().clone(true).appendTo($("#dns_server_list2"));
			$(".dns_pull_arrow").show();
		}
	);
	$("body").click(function() {
		$(".dns_pull_arrow").attr("src","/images/arrow-down.gif");
		$(".dns_server_list_dropdown").hide();
	});
}

function change_wan_unit(obj){
	if(!dualWAN_support) return;

	if(obj.options[obj.selectedIndex].text == "DSL"){
		if(document.form.dsltmp_transmode){
			document.form.dsltmp_transmode.style.display = "";
			//change_dsl_transmode(document.form.dsltmp_transmode);
		}

		if(document.form.dsltmp_transmode.value == document.form.dslx_transmode.value)
			document.getElementById("active_dslmode").innerHTML = "( Active )";
		else
			document.getElementById("active_dslmode").innerHTML = "( Not Active )";

	}else if(document.form.dsltmp_transmode){
		document.form.dsltmp_transmode.style.display = "none";
	}

	if(obj.options[obj.selectedIndex].text == "WAN" 
		|| obj.options[obj.selectedIndex].text == "Ethernet LAN"
		|| obj.options[obj.selectedIndex].text == "Ethernet WAN"){
		document.form.current_page.value = "Advanced_WAN_Content.asp";
	}
	else if(obj.options[obj.selectedIndex].text == "USB") {
		document.form.current_page.value = "Advanced_Modem_Content.asp";
	}
	else if(obj.options[obj.selectedIndex].text == "DSL" && document.form.dsltmp_transmode.value == "atm"){
		document.form.current_page.value = "Advanced_DSL_Content.asp";
	}else{
		return false;
	}

	FormActions("apply.cgi", "change_wan_unit", "", "");
	document.form.target = "";
	document.form.submit();
}

function change_dsl_transmode(obj){
	if(obj.value == "atm"){
		document.form.current_page.value = "Advanced_DSL_Content.asp";
	}else{ // ptm
		return false;
	}
	FormActions("apply.cgi", "change_dslx_transmode", "", "");
	document.form.target = "";
	document.form.submit();
}

function genWANSoption(){
	if(!dualWAN_support) return;

	for(i=0; i<wans_dualwan.split(" ").length; i++){
		var wans_dualwan_NAME = wans_dualwan.split(" ")[i].toUpperCase();
		//MODELDEP: DSL-N55U, DSL-N55U-B, DSL-AC68U, DSL-AC68R
		if(wans_dualwan_NAME == "LAN" && 
				(productid == "DSL-N55U" || productid == "DSL-N55U-B" || productid == "DSL-AC68U" || productid == "DSL-AC68R")) 
				wans_dualwan_NAME = "Ethernet WAN";
		else if(wans_dualwan_NAME == "LAN")
				wans_dualwan_NAME = "Ethernet LAN";
		if(wans_dualwan_NAME != "NONE")
			document.form.wan_unit.options[i] = new Option(wans_dualwan_NAME, i);
	}
	document.form.wan_unit.selectedIndex = load_wan_unit;
}

function change_dsl_unit_idx(idx,iptv_row){
	// reset to old values
	if (iptv_row == "0") document.getElementById("pvc_sel").innerHTML = "Internet PVC";
	else document.getElementById("pvc_sel").innerHTML = "IPTV PVC #"+iptv_row.toString();

	document.form.dsl_unit.value = "8";
	document.form.dsl_subunit.value = iptv_row;
	document.form.dsl_enable.value = DSLWANList[iptv_row][0];
	document.form.dsl_proto.value = DSLWANList[iptv_row][1];
	document.form.dsl_dot1q.value = DSLWANList[iptv_row][2];
	document.form.dsl_vid.value = DSLWANList[iptv_row][3];
	document.form.dsl_dot1p.value = DSLWANList[iptv_row][4];

	document.getElementById("dslSettings").style.display = "";
	if (document.form.dsl_proto.value != "bridge") {
		document.getElementById("IPsetting").style.display = "";
		document.getElementById("DNSsetting").style.display = "";
		document.getElementById("PPPsetting").style.display = "";
		document.getElementById("vpn_server").style.display = "";
	}
	else {
		document.getElementById("IPsetting").style.display = "none";
		document.getElementById("DNSsetting").style.display = "none";
		document.getElementById("PPPsetting").style.display = "none";
		document.getElementById("vpn_server").style.display = "none";
	}
}

function del_pvc_submit(){
	showLoading();
	document.form.submit();
}

function exit_to_main(){
	//enable_pvc_summary();
	//disable_all_ctrl();
	location.href = "Advanced_VDSL_Content.asp";
}

function applyRule(){
	if(validForm()){

		if (document.form.dsl_proto.value == "bridge") {
			document.getElementById('bridgePPPoE_relay').innerHTML = '<input type="hidden" name="fw_pt_pppoerelay" value="1"> ';
			if(document.form.dsl_dot1q.value == "1")
				document.form.dslx_rmvlan.value = "1";
		}
/*
		if (document.form.dsl_unit.value != 0 && document.form.add_pvc_flag.value == "1" && 
			(productid == "DSL-AC68U" || productid == "DSL-AC68R")	//Add PVC #1~#7 need "reboot" for DSL-AC68U
		){
			document.form.action_script.value = "reboot";		
			document.form.action_wait.value = "<% get_default_reboot_time(); %>";				
		}
*/
		if (document.form.dsl_proto.value == "static"){
			document.form.dsl_DHCPClient.value = 0;
			document.form.dsl_dnsenable[1].disabled = false;
			document.form.dsl_dnsenable[1].checked = true;
		}
		else if(document.form.dsl_DHCPClient_x){
			if(document.form.dsl_DHCPClient_x[0].checked == 1){
				document.form.dsl_DHCPClient.value = 1;
			}
			else{
				document.form.dsl_DHCPClient.value = 0;
				document.form.dsl_dnsenable[1].disabled = false;
				document.form.dsl_dnsenable[1].checked = true;	
			}
		}

		if(dnspriv_support){
			if(document.form.dnspriv_enable.value == 1 && document.form.dsl_subunit.value == 0){
				var dnspriv_rulelist_value = "";
				for(k=0; k<document.getElementById('dnspriv_rulelist_table').rows.length; k++){
					for(j=0; j<document.getElementById('dnspriv_rulelist_table').rows[k].cells.length-1; j++){
						if(j == 0)
							dnspriv_rulelist_value += "<";
						else
							dnspriv_rulelist_value += ">";
						dnspriv_rulelist_value += document.getElementById('dnspriv_rulelist_table').rows[k].cells[j].innerHTML;
					}
				}
				document.form.dnspriv_rulelist.disabled = false;
				document.form.dnspriv_rulelist.value = dnspriv_rulelist_value;
			}
			document.form.action_script.value += ";restart_stubby";
		}

		document.form.wan_enable.value = document.form.dsl_link_enable.value;
		document.form.dsl_enable.value = "1";
		if(document.form.dslx_transmode.value != document.form.dsltmp_transmode.value) {
			if (based_modelid == "DSL-AC68U")
				document.form.action_script.value = "restart_dsl_setting;".concat(document.form.action_script.value);
			document.form.dslx_transmode.value = document.form.dsltmp_transmode.value;
		}
		showLoading();
		document.form.submit();
	}
}

// test if WAN IP & Gateway & DNS IP is a valid IP
// DNS IP allows to input nothing
function valid_IP(obj_name, obj_flag){
		// A : 1.0.0.0~126.255.255.255
		// B : 127.0.0.0~127.255.255.255 (forbidden)
		// C : 128.0.0.0~255.255.255.254
		var A_class_start = inet_network("1.0.0.0");
		var A_class_end = inet_network("126.255.255.255");
		var B_class_start = inet_network("127.0.0.0");
		var B_class_end = inet_network("127.255.255.255");
		var C_class_start = inet_network("128.0.0.0");
		var C_class_end = inet_network("255.255.255.255");

		var ip_obj = obj_name;
		var ip_num = inet_network(ip_obj.value);

		if(obj_flag == "DNS" && ip_num == -1){ //DNS allows to input nothing
			return true;
		}

		if(obj_flag == "GW" && ip_num == -1){ //GW allows to input nothing
			return true;
		}

		if(ip_num > A_class_start && ip_num < A_class_end)
			return true;
		else if(ip_num > B_class_start && ip_num < B_class_end){
			alert(ip_obj.value+" <#JS_validip#>");
			ip_obj.focus();
			ip_obj.select();
			return false;
		}
		else if(ip_num > C_class_start && ip_num < C_class_end)
			return true;
		else{
			alert(ip_obj.value+" <#JS_validip#>");
			ip_obj.focus();
			ip_obj.select();
			return false;
		}
}

function validForm(){
	if(!document.form.dsl_DHCPClient_x[0].checked){// Set IP address by userself
		if(!valid_IP(document.form.dsl_ipaddr, "")) return false;  //WAN IP
		if(!valid_IP(document.form.dsl_gateway, "GW"))return false;  //Gateway IP

		if(document.form.dsl_gateway.value == document.form.dsl_ipaddr.value){
			alert("<#IPConnection_warning_WANIPEQUALGatewayIP#>");
			return false;
		}

		// test if netmask is valid.
		var default_netmask = "";
		var wrong_netmask = 0;
		var netmask_obj = document.form.dsl_netmask;
		var netmask_num = inet_network(netmask_obj.value);

		if(netmask_num==0){
			var netmask_reverse_num = 0;		//Viz 2011.07 : Let netmask 0.0.0.0 pass
		}else{
		var netmask_reverse_num = ~netmask_num;
		}

		if(netmask_num < 0) wrong_netmask = 1;

		var test_num = netmask_reverse_num;
		while(test_num != 0){
			if((test_num+1)%2 == 0)
				test_num = (test_num+1)/2-1;
			else{
				wrong_netmask = 1;
				break;
			}
		}
		if(wrong_netmask == 1){
			alert(netmask_obj.value+" <#JS_validip#>");
			netmask_obj.value = default_netmask;
			netmask_obj.focus();
			netmask_obj.select();
			return false;
		}
	}

	if(!document.form.dsl_dnsenable[0].checked){
		if(!valid_IP(document.form.dsl_dns1, "DNS")) return false;  //DNS1
		if(!valid_IP(document.form.dsl_dns2, "DNS")) return false;  //DNS2
	}

	if(document.form.dsl_proto.value == "pppoe"
			|| document.form.dsl_proto.value == "pppoa"
			){
		if(!validator.string(document.form.dsl_pppoe_username)
				|| !validator.string(document.form.dsl_pppoe_passwd)
				)
			return false;

		if(!validator.numberRange(document.form.dsl_pppoe_idletime, 0, 4294967295))
			return false;
	}

	if(document.form.dsl_proto.value == "pppoe"){
		if(!validator.numberRange(document.form.dsl_pppoe_mtu, 128, 1492)
			|| !validator.numberRange(document.form.dsl_pppoe_mru, 128, 1492))
			return false;

		if(document.form.dsl_mtu.value != "") {
			if(document.form.dsl_pppoe_mtu.value + 8 > document.form.dsl_mtu.value) {
				document.form.dsl_pppoe_mtu.value = document.form.dsl_mtu.value - 8;
			}
			if(document.form.dsl_pppoe_mru.value + 8 > document.form.dsl_mtu.value) {
				document.form.dsl_pppoe_mru.value = document.form.dsl_mtu.value - 8;
			}
		}

		if(!validator.string(document.form.dsl_pppoe_service)
				|| !validator.string(document.form.dsl_pppoe_ac))
			return false;

		//pppoe hostuniq
		if(!validator.hex(document.form.dsl_pppoe_hostuniq)) {
			alert("Host-uniq should be hexadecimal digits.");
			document.form.dsl_pppoe_hostuniq.focus();
			document.form.dsl_pppoe_hostuniq.select();
			return false;
		}
	}

	if(document.form.dsl_hwaddr.value.length > 0)
		if(!check_macaddr(document.form.dsl_hwaddr, check_hwaddr_flag(document.form.dsl_hwaddr))){
				document.form.dsl_hwaddr.select();
				document.form.dsl_hwaddr.focus();
				return false;
		}

	if(orig_mtu != "" || document.form.dsl_mtu.value.length > 0) {
		if(!validator.numberRange(document.form.dsl_mtu, 1280, 1500)) {
			document.form.dsl_mtu.focus();
			document.form.dsl_mtu.select();
			return false;
		}
	}

	return true;
}

function done_validating(action){
	refreshpage();
}

function disable_pvc_summary() {
	document.getElementById("DSL_WAN_table").style.display = "none";
}

function enable_pvc_summary() {
	document.getElementById("DSL_WAN_table").style.display = "";
}

function disable_all_ctrl() {
	document.getElementById("desc_default").style.display = "";
	document.getElementById("desc_edit").style.display = "none";
	document.getElementById("dslSettings").style.display = "none";
	document.getElementById("PPPsetting").style.display = "none";
	document.getElementById("DNSsetting").style.display = "none";
	document.getElementById("dot1q_setting").style.display = "none";
	document.getElementById("IPsetting").style.display = "none";
	document.getElementById("DHCP_option").style.display = "none";
	document.getElementById("vpn_server").style.display = "none";
	document.getElementById("btn_apply").style.display = "none";
}

function enable_all_ctrl(pvc) {
	document.getElementById("desc_default").style.display = "none";
	document.getElementById("desc_edit").style.display = "";
	document.getElementById("dslSettings").style.display = "";
	document.getElementById("PPPsetting").style.display = "";
	document.getElementById("DNSsetting").style.display = "";
	document.getElementById("dot1q_setting").style.display = "";
	document.getElementById("IPsetting").style.display = "";
	document.getElementById("DHCP_option").style.display = "";
	document.getElementById("vpn_server").style.display = "";
	document.getElementById("btn_apply").style.display = "";

	if(dnspriv_support && pvc == 0){
		inputCtrl(document.form.dnspriv_enable, 1);
		change_dnspriv_enable(document.form.dnspriv_enable.value);
	}
	else{
		inputCtrl(document.form.dnspriv_enable, 0);
		change_dnspriv_enable(0);
	}
}

function change_dsl_type(dsl_type){
	change_dsl_dhcp_enable();
	change_dsl_dns_enable();

	if(dsl_type == "pppoe" || dsl_type == "pppoa"){
		//inputCtrl(document.form.dsl_dnsenable[0], 1);
		//inputCtrl(document.form.dsl_dnsenable[1], 1);
		showhide("DHCP_option",0);
		inputCtrl(document.form.dsl_dhcp_vendorid, 0);
		inputCtrl(document.form.dsl_dhcp_clientid, 0);
		document.form.dsl_dhcp_clientid_type.disabled = true;

		inputCtrl(document.form.dsl_pppoe_username, 1);
		inputCtrl(document.form.dsl_pppoe_passwd, 1);
		inputCtrl(document.form.dsl_pppoe_auth, 1);
		inputCtrl(document.form.dsl_pppoe_idletime, 1);
		inputCtrl(document.form.dsl_pppoe_mtu, 1);
		inputCtrl(document.form.dsl_pppoe_mru, 1);
		inputCtrl(document.form.dsl_pppoe_service, 1);
		inputCtrl(document.form.dsl_pppoe_ac, 1);

		// 2008.03 James. patch for Oleg's patch. {
		inputCtrl(document.form.dsl_pppoe_options, 1);
		// 2008.03 James. patch for Oleg's patch. }
		showhide("PPPsetting",1);
		inputCtrl(document.form.wan_ppp_echo, 1);
		ppp_echo_control();
		inputCtrl(document.form.dsl_dhcp_qry, 0);
	}
	else if(dsl_type == "static"){
		//inputCtrl(document.form.dsl_dnsenable[0], 0);
		//inputCtrl(document.form.dsl_dnsenable[1], 0);
		showhide("DHCP_option",0);
		inputCtrl(document.form.dsl_dhcp_vendorid, 0);
		inputCtrl(document.form.dsl_dhcp_clientid, 0);
		document.form.dsl_dhcp_clientid_type.disabled = true;

		inputCtrl(document.form.dsl_pppoe_username, 0);
		inputCtrl(document.form.dsl_pppoe_passwd, 0);
		inputCtrl(document.form.dsl_pppoe_auth, 0);
		inputCtrl(document.form.dsl_pppoe_idletime, 0);
		inputCtrl(document.form.dsl_pppoe_mtu, 0);
		inputCtrl(document.form.dsl_pppoe_mru, 0);
		inputCtrl(document.form.dsl_pppoe_service, 0);
		inputCtrl(document.form.dsl_pppoe_ac, 0);
		// 2008.03 James. patch for Oleg's patch. {
		inputCtrl(document.form.dsl_pppoe_options, 0);
		// 2008.03 James. patch for Oleg's patch. }
		showhide("PPPsetting",0);
		inputCtrl(document.form.wan_ppp_echo, 0);
		ppp_echo_control(0);
		inputCtrl(document.form.dsl_dhcp_qry, 0);
	}
	else if(dsl_type == "dhcp"){
		//inputCtrl(document.form.dsl_dnsenable[0], 1);
		//inputCtrl(document.form.dsl_dnsenable[1], 1);
		showhide("DHCP_option",1);
		inputCtrl(document.form.dsl_dhcp_vendorid, 1);
		inputCtrl(document.form.dsl_dhcp_clientid, 1);
		document.form.dsl_dhcp_clientid_type.disabled = false;
		showDiableDHCPclientID(document.form.tmp_dhcp_clientid_type);

		inputCtrl(document.form.dsl_pppoe_username, 0);
		inputCtrl(document.form.dsl_pppoe_passwd, 0);
		inputCtrl(document.form.dsl_pppoe_auth, 0);
		inputCtrl(document.form.dsl_pppoe_idletime, 0);
		inputCtrl(document.form.dsl_pppoe_mtu, 0);
		inputCtrl(document.form.dsl_pppoe_mru, 0);
		inputCtrl(document.form.dsl_pppoe_service, 0);
		inputCtrl(document.form.dsl_pppoe_ac, 0);

		// 2008.03 James. patch for Oleg's patch. {
		inputCtrl(document.form.dsl_pppoe_options, 0);
		// 2008.03 James. patch for Oleg's patch. }
		showhide("PPPsetting",0);
		inputCtrl(document.form.wan_ppp_echo, 0);
		ppp_echo_control(0);
		inputCtrl(document.form.dsl_dhcp_qry, 1);
	}
	else if(dsl_type == "bridge") {
		//inputCtrl(document.form.dsl_dnsenable[0], 0);
		//inputCtrl(document.form.dsl_dnsenable[1], 0);
		showhide("DHCP_option",0);
		inputCtrl(document.form.dsl_dhcp_vendorid, 0);
		inputCtrl(document.form.dsl_dhcp_clientid, 0);
		document.form.dsl_dhcp_clientid_type.disabled = true;
		inputCtrl(document.form.dsl_pppoe_username, 0);
		inputCtrl(document.form.dsl_pppoe_passwd, 0);
		inputCtrl(document.form.dsl_pppoe_auth, 0);
		inputCtrl(document.form.dsl_pppoe_idletime, 0);
		inputCtrl(document.form.dsl_pppoe_mtu, 0);
		inputCtrl(document.form.dsl_pppoe_mru, 0);
		inputCtrl(document.form.dsl_pppoe_service, 0);
		inputCtrl(document.form.dsl_pppoe_ac, 0);

		// 2008.03 James. patch for Oleg's patch. {
		inputCtrl(document.form.dsl_pppoe_options, 0);
		// 2008.03 James. patch for Oleg's patch. }
		showhide("PPPsetting",0);
		inputCtrl(document.form.wan_ppp_echo, 0);
		ppp_echo_control(0);
		inputCtrl(document.form.dsl_dhcp_qry, 0);
	}
	else {
		alert("error");
	}
}


function fixed_change_dsl_type(dsl_type){
	if(!document.form.dsl_DHCPClient_x[0].checked){
		if(document.form.dsl_ipaddr.value.length == 0)
			document.form.dsl_ipaddr.focus();
		else if(document.form.dsl_netmask.value.length == 0)
			document.form.dsl_netmask.focus();
		else if(document.form.dsl_gateway.value.length == 0)
			document.form.dsl_gateway.focus();
	}

	if(dsl_type == "pppoe" || dsl_type == "pppoa"){
		document.form.dsl_dnsenable[0].checked = original_dnsenable;
		document.form.dsl_dnsenable[1].checked = !original_dnsenable;
		change_common_radio(document.form.dsl_dnsenable, 'IPConnection', 'dsl_dnsenable', 0);
		inputCtrl(document.form.dsl_dns1, !original_dnsenable);
		inputCtrl(document.form.dsl_dns2, !original_dnsenable);
		inputCtrl(document.form.dsl_hwaddr, 1);
		inputCtrl(document.form.dsl_link_enable[0], 1);
		inputCtrl(document.form.dsl_link_enable[1], 1);
		inputCtrl(document.form.dsl_nat[0], 1);
		inputCtrl(document.form.dsl_nat[1], 1);
		inputCtrl(document.form.dsl_upnp_enable[0], 1);
		inputCtrl(document.form.dsl_upnp_enable[1], 1);
		showhide("IPsetting",1);
		showhide("DNSsetting",1);
		showhide("vpn_server",1);
		document.form.wan_ppp_echo.value = original_ppp_echo;
		ppp_echo_control();
	}
	//else if(dsl_type == "ipoa"){
	else if(dsl_type == "static"){
		document.form.dsl_dnsenable[0].checked = 0;
		document.form.dsl_dnsenable[1].checked = 1;
		change_common_radio(document.form.dsl_dnsenable, 'IPConnection', 'dsl_dnsenable', 0);
		inputCtrl(document.form.dsl_hwaddr, 1);
		inputCtrl(document.form.dsl_link_enable[0], 1);
		inputCtrl(document.form.dsl_link_enable[1], 1);
		inputCtrl(document.form.dsl_nat[0], 1);
		inputCtrl(document.form.dsl_nat[1], 1);
		inputCtrl(document.form.dsl_upnp_enable[0], 1);
		inputCtrl(document.form.dsl_upnp_enable[1], 1);
		showhide("IPsetting",1);
		showhide("DNSsetting",1);
		showhide("vpn_server",1);
	}	
	else if(dsl_type == "dhcp"){
		inputCtrl(document.form.dsl_DHCPClient_x[0], 0);
		inputCtrl(document.form.dsl_DHCPClient_x[1], 0);
		document.getElementById("IPsetting").style.display = "none";
		inputCtrl(document.form.dsl_hwaddr, 1);
		inputCtrl(document.form.dsl_link_enable[0], 1);
		inputCtrl(document.form.dsl_link_enable[1], 1);
		inputCtrl(document.form.dsl_nat[0], 1);
		inputCtrl(document.form.dsl_nat[1], 1);
		inputCtrl(document.form.dsl_upnp_enable[0], 1);
		inputCtrl(document.form.dsl_upnp_enable[1], 1);
		showhide("DNSsetting",1);
		showhide("vpn_server",1);
	}
	else if(dsl_type == "bridge"){
		document.form.dsl_dnsenable[0].checked = 1;
		document.form.dsl_dnsenable[1].checked = 0;
		change_common_radio(document.form.dsl_dnsenable, 'IPConnection', 'dsl_dnsenable', 0);

		inputCtrl(document.form.dsl_dns1, 0);
		inputCtrl(document.form.dsl_dns2, 0);
		inputCtrl(document.form.dsl_hwaddr, 0);
		if(document.form.dsl_subunit.value != 0){
			inputCtrl(document.form.dsl_link_enable[0], 0);
			inputCtrl(document.form.dsl_link_enable[1], 0);
			inputCtrl(document.form.dsl_upnp_enable[0], 0);
			inputCtrl(document.form.dsl_upnp_enable[1], 0);
		}
		else{
			inputCtrl(document.form.dsl_link_enable[0], 1);
			inputCtrl(document.form.dsl_link_enable[1], 1);
			inputCtrl(document.form.dsl_upnp_enable[0], 1);
			inputCtrl(document.form.dsl_upnp_enable[1], 1);
		}
		inputCtrl(document.form.dsl_nat[0], 0);
		inputCtrl(document.form.dsl_nat[1], 0);
		showhide("DNSsetting",0);
		showhide("vpn_server",0);
	}
	else {
		alert("error");
	}
}

function change_dsl_dns_enable(){
	var dsl_type = document.form.dsl_proto.value;

	if(dsl_type == "pppoe" || dsl_type == "pppoa" || dsl_type == "dhcp"){
		inputCtrl(document.form.dsl_dnsenable[0], 1);
		inputCtrl(document.form.dsl_dnsenable[1], 1);

		var wan_dnsenable = document.form.dsl_dnsenable[0].checked;

		inputCtrl(document.form.dsl_dns1, !wan_dnsenable);
		inputCtrl(document.form.dsl_dns2, !wan_dnsenable);
	}
	else if(dsl_type == "static"){
	// value fix
		inputCtrl(document.form.dsl_dnsenable[0], 0);
		inputCtrl(document.form.dsl_dnsenable[1], 0);

		inputCtrl(document.form.dsl_dns1, 1);
		inputCtrl(document.form.dsl_dns2, 1);
	}
	else{	// dsl_type == bridge
		inputCtrl(document.form.dsl_dnsenable[0], 0);
		inputCtrl(document.form.dsl_dnsenable[1], 0);

		inputCtrl(document.form.dsl_dns1, 0);
		inputCtrl(document.form.dsl_dns2, 0);
	}

	if(document.form.dsl_DHCPClient_x[0].checked){
		inputCtrl(document.form.dsl_dnsenable[0], 1);
		inputCtrl(document.form.dsl_dnsenable[1], 1);
	}
	else{		// dhcp NO
		document.form.dsl_dnsenable[0].checked = 0;
		document.form.dsl_dnsenable[1].checked = 1;
		change_common_radio(document.form.dsl_dnsenable, 'IPConnection', 'dsl_dnsenable', 0);

		inputCtrl(document.form.dsl_dnsenable[0], 1);
		inputCtrl(document.form.dsl_dnsenable[1], 1);
	}

}

function change_dsl_dhcp_enable(){
	var dsl_type = document.form.dsl_proto.value;

	if(dsl_type == "pppoe" || dsl_type == "pppoa"){
		inputCtrl(document.form.dsl_DHCPClient_x[0], 1);
		inputCtrl(document.form.dsl_DHCPClient_x[1], 1);

		var wan_dhcpenable = document.form.dsl_DHCPClient_x[0].checked;

		inputCtrl(document.form.dsl_ipaddr, !wan_dhcpenable);
		inputCtrl(document.form.dsl_netmask, !wan_dhcpenable);
		inputCtrl(document.form.dsl_gateway, !wan_dhcpenable);
		document.getElementById("IPsetting").style.display = "";

		if(document.form.dsl_DHCPClient_x[1].checked)
		{
			inputCtrl(document.form.dsl_dns1, 1);
			inputCtrl(document.form.dsl_dns2, 1);
		}
	}
	else if(dsl_type == "dhcp"){	//Viz modify this case
		document.form.dsl_DHCPClient_x[0].checked = 1;

		inputCtrl(document.form.dsl_ipaddr, 0);
		inputCtrl(document.form.dsl_netmask, 0);
		inputCtrl(document.form.dsl_gateway, 0);
		document.getElementById("IPsetting").style.display = "none";

		if(document.form.dsl_dnsenable[1].checked)
		{
			inputCtrl(document.form.dsl_dns1, 1);
			inputCtrl(document.form.dsl_dns2, 1);
		}
	}
	else if(dsl_type == "static"){
		document.form.dsl_DHCPClient_x[0].checked = 0;
		document.form.dsl_DHCPClient_x[1].checked = 1;
		inputCtrl(document.form.dsl_DHCPClient_x[0], 0);
		inputCtrl(document.form.dsl_DHCPClient_x[1], 0);
		inputCtrl(document.form.dsl_ipaddr, 1);
		inputCtrl(document.form.dsl_netmask, 1);
		inputCtrl(document.form.dsl_gateway, 1);
		document.getElementById("IPsetting").style.display = "";
	}
	else{	// dsl_type == bridge
		document.form.dsl_DHCPClient_x[0].checked = 1;
		document.form.dsl_DHCPClient_x[1].checked = 0;
		inputCtrl(document.form.dsl_DHCPClient_x[0], 0);
		inputCtrl(document.form.dsl_DHCPClient_x[1], 0);
		inputCtrl(document.form.dsl_ipaddr, 0);
		inputCtrl(document.form.dsl_netmask, 0);
		inputCtrl(document.form.dsl_gateway, 0);
		document.getElementById("IPsetting").style.display = "none";
	}

	if(document.form.dsl_DHCPClient_x[0].checked){
		inputCtrl(document.form.dsl_dnsenable[0], 1);
		inputCtrl(document.form.dsl_dnsenable[1], 1);
	}
	else{		// dhcp NO
		document.form.dsl_dnsenable[0].checked = 0;
		document.form.dsl_dnsenable[1].checked = 1;
		change_common_radio(document.form.dsl_dnsenable, 'IPConnection', 'dsl_dnsenable', 0);

		inputCtrl(document.form.dsl_dnsenable[0], 0);
		inputCtrl(document.form.dsl_dnsenable[1], 0);
	}
}

function showMAC(){
	var tempMAC = "";
	document.form.dsl_hwaddr.value = login_mac_str();
}

function check_macaddr(obj,flag){ //control hint of input mac address
	if(flag == 1){
		var childsel=document.createElement("div");
		childsel.setAttribute("id","check_mac");
		childsel.style.color="#FFCC00";
		obj.parentNode.appendChild(childsel);
		document.getElementById("check_mac").innerHTML="<#LANHostConfig_ManualDHCPMacaddr_itemdesc#>";
		return false;
	}else if(flag ==2){
		var childsel=document.createElement("div");
		childsel.setAttribute("id","check_mac");
		childsel.style.color="#FFCC00";
		obj.parentNode.appendChild(childsel);
		document.getElementById("check_mac").innerHTML="<#IPConnection_x_illegal_mac#>";
		return false;
	}else{
		document.getElementById("check_mac") ? document.getElementById("check_mac").style.display="none" : true;
		return true;
	}
}

/* password item show or not */
function pass_checked(obj){
	switchType(obj, document.form.show_pass_1.checked, true);
}

function ppp_echo_control(flag){
	if (typeof(flag) == 'undefined')
		flag = document.form.wan_ppp_echo.value;
	var enable = (flag == 1) ? 1 : 0;
	inputCtrl(document.form.wan_ppp_echo_interval, enable);
	inputCtrl(document.form.wan_ppp_echo_failure, enable);
	enable = (flag == 2) ? 1 : 0;
	//inputCtrl(document.form.dns_probe_timeout, enable);
	inputCtrl(document.form.dns_delay_round, enable);
}

function change_dnspriv_enable(flag){
	if(flag == 1){
		inputCtrl(document.form.dnspriv_profile[0], 1);
		inputCtrl(document.form.dnspriv_profile[1], 1);
		document.getElementById("DNSPrivacy").style.display = "";
		document.getElementById("dnspriv_rulelist_Block").style.display = "";
		show_dnspriv_rulelist();
	}
	else{
		inputCtrl(document.form.dnspriv_profile[0], 0);
		inputCtrl(document.form.dnspriv_profile[1], 0);
		document.getElementById("DNSPrivacy").style.display = "none";
		document.getElementById("dnspriv_rulelist_Block").style.display = "none";
	}
}

function addRow(obj, head){
	if(head == 1)
		dnspriv_rulelist_array += "&#60"
	else
		dnspriv_rulelist_array += "&#62"

	dnspriv_rulelist_array += obj.value;
	obj.value = "";
}

function addRow_Group(upper){
	var rule_num = document.getElementById('dnspriv_rulelist_table').rows.length;
	var item_num = document.getElementById('dnspriv_rulelist_table').rows[0].cells.length;		
	if(rule_num >= upper){
		alert("<#JS_itemlimit1#> " + upper + " <#JS_itemlimit2#>");
		return false;	
	}	

	if(document.form.dnspriv_server_0.value==""){
		alert("<#JS_fieldblank#>");
		document.form.dnspriv_server_0.focus();
		document.form.dnspriv_server_0.select();		
		return false;
	}
	else{
		addRow(document.form.dnspriv_server_0, 1);
		addRow(document.form.dnspriv_port_0, 0);
		addRow(document.form.dnspriv_hostname_0, 0);
		addRow(document.form.dnspriv_spkipin_0, 0);
		show_dnspriv_rulelist();
	}
}

function edit_Row(r){ 	
	var i=r.parentNode.parentNode.rowIndex;
  	document.form.dnspriv_server_0.value = document.getElementById('dnspriv_rulelist_table').rows[i].cells[0].innerHTML;
	document.form.dnspriv_port_0.value = document.getElementById('dnspriv_rulelist_table').rows[i].cells[1].innerHTML; 
	document.form.dnspriv_hostname_0.value = document.getElementById('dnspriv_rulelist_table').rows[i].cells[2].innerHTML; 
	document.form.dnspriv_spkipin_0.value = document.getElementById('dnspriv_rulelist_table').rows[i].cells[3].innerHTML;

	del_Row(r);	
}

function del_Row(r){
	var i=r.parentNode.parentNode.rowIndex;
	document.getElementById('dnspriv_rulelist_table').deleteRow(i);

	var dnspriv_rulelist_value = "";
	for(k=0; k<document.getElementById('dnspriv_rulelist_table').rows.length; k++){
		for(j=0; j<document.getElementById('dnspriv_rulelist_table').rows[k].cells.length-1; j++){
			if(j == 0)
				dnspriv_rulelist_value += "&#60";
			else
				dnspriv_rulelist_value += "&#62";
			dnspriv_rulelist_value += document.getElementById('dnspriv_rulelist_table').rows[k].cells[j].innerHTML;
		}
	}

	dnspriv_rulelist_array = dnspriv_rulelist_value;
	if(dnspriv_rulelist_array == "")
		show_dnspriv_rulelist();
}

function show_dnspriv_rulelist(){
	var dnspriv_rulelist_row = dnspriv_rulelist_array.split('&#60');
	var code = "";

	code +='<table width="100%" border="1" cellspacing="0" cellpadding="4" align="center" class="list_table" id="dnspriv_rulelist_table">';
	if(dnspriv_rulelist_row.length == 1)
		code +='<tr><td style="color:#FFCC00;" colspan="5"><#IPConnection_VSList_Norule#></td></tr>';
	else{
		for(var i = 1; i < dnspriv_rulelist_row.length; i++){
			code +='<tr id="row'+i+'">';
			var dnspriv_rulelist_col = dnspriv_rulelist_row[i].split('&#62');
			var wid=[27, 10, 27, 27];
				for(var j = 0; j < dnspriv_rulelist_col.length; j++){
					code +='<td width="'+wid[j]+'%">'+ dnspriv_rulelist_col[j] +'</td>';
				}
				code +='<td width="9%"><!--input class="edit_btn" onclick="edit_Row(this);" value=""/-->';
				code +='<input class="remove_btn" onclick="del_Row(this);" value=""/></td></tr>';
		}
	}
	code +='</table>';
	document.getElementById("dnspriv_rulelist_Block").innerHTML = code;
}

function pullDNSList(_this) {
	event.stopPropagation();
	var idx = $(_this).attr("id").replace("dns_pull_arrow", "");
	$(".dns_pull_arrow:not(#dns_pull_arrow" + idx + ")").attr("src","/images/arrow-down.gif");
	$(".dns_server_list_dropdown:not(#dns_server_list" + idx + ")").hide();
	var $element = $("#dns_server_list" + idx + "");
	var isMenuopen = $element[0].offsetWidth > 0 || $element[0].offsetHeight > 0;
	if(isMenuopen == 0) {
		$(_this).attr("src","/images/arrow-top.gif");
		$element.show();
	}
	else {
		$(_this).attr("src","/images/arrow-down.gif");
		$element.hide();
	}
}

function showDiableDHCPclientID(clientid_enable){
	if(clientid_enable.checked) {
		document.form.dsl_dhcp_clientid_type.value = "1";
		document.form.dsl_dhcp_clientid.value = "";
		document.form.dsl_dhcp_clientid.style.display = "none";
	}
	else {
		document.form.dsl_dhcp_clientid_type.value = "0";
		document.form.dsl_dhcp_clientid.style.display = "";
	}
}

</script>
</head>

<body onload="initial();" onunLoad="return unload_body();">
<div id="TopBanner"></div>
<div id="Loading" class="popup_bg"></div>
<div id="hiddenMask" class="popup_bg">
	<table cellpadding="5" cellspacing="0" id="dr_sweet_advise" class="dr_sweet_advise" align="center" style="height:100px;">
		<tr>
		<td>
			<div class="drword" id="drword" style=""></div>
		</td>
		</tr>
	</table>
<!--[if lte IE 6.5]><iframe class="hackiframe"></iframe><![endif]-->
</div>
<iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>
<form method="post" name="form" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="productid" value="<% nvram_get("productid"); %>">
<input type="hidden" name="current_page" value="Advanced_VDSL_Content.asp">
<input type="hidden" name="next_page" value="Advanced_VDSL_Content.asp">
<input type="hidden" name="group_id" value="">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="restart_dslwan_if">
<input type="hidden" name="action_wait" value="5">
<input type="hidden" name="first_time" value="">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="lan_ipaddr" value="<% nvram_get("lan_ipaddr"); %>" />
<input type="hidden" name="lan_netmask" value="<% nvram_get("lan_netmask"); %>" />
<input type="hidden" name="dsl_unit" value="8" />
<input type="hidden" name="dsl_subunit" value="" />
<input type="hidden" name="dsl_enable" value="" />
<input type="hidden" name="dslx_transmode" value="<% nvram_get("dslx_transmode"); %>">
<input type="hidden" name="dsl_DHCPClient" value="<% nvram_get("dsl_DHCPClient"); %>">
<input type="hidden" name="dslx_rmvlan" value="<% nvram_get("dslx_rmvlan"); %>">
<input type="hidden" name="wan_enable" value="" disabled>
<input type="hidden" name="add_pvc_flag" value="0">
<input type="hidden" name="dsl_dhcp_clientid_type" value="">
<input type="hidden" name="dnspriv_rulelist" value="<% nvram_get("dnspriv_rulelist"); %>" disabled>
<span id="bridgePPPoE_relay"></span>
<table class="content" align="center" cellpadding="0" cellspacing="0">
	<tr>
		<td width="17">&nbsp;</td>
	<!--=====Beginning of Main Menu=====-->
		<td valign="top" width="202">
		  <div id="mainMenu"></div>
		  <div id="subMenu"></div>
		</td>

		<td height="430" valign="top">
			<div id="tabMenu" class="submenuBlock"></div>
		  <!--===================================Beginning of Main Content===========================================-->
			<table width="98%" border="0" align="left" cellpadding="0" cellspacing="0">
				<tr>
					<td valign="top">
						<table width="760px" border="0" cellpadding="5" cellspacing="0" class="FormTitle" id="FormTitle">
							<tbody>
								<tr>
								<td bgcolor="#4D595D" valign="top">
									<div>&nbsp;</div>
									<div class="formfonttitle"><#menu5_3#> - <#menu5_3_1#></div>
									<div style="margin: 10px 0 10px 5px;" class="splitLine"></div>
									<div id="desc_default" class="formfontdesc"><#dsl_wan_page_desc#></div>
									<div id="desc_edit" class="formfontdesc"><#Layer3Forwarding_x_ConnectionType_sectiondesc#></div>

									<table id="WANscap" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
										<thead>
										<tr>
											<td colspan="2"><#wan_index#></td>
										</tr>
										</thead>
										<tr>
											<th><#wan_type#></th>
											<td align="left">
												<select id="wan_unit" class="input_option" name="wan_unit" onchange="change_wan_unit(this);"></select>
												<select id="dsltmp_transmode" name="dsltmp_transmode" class="input_option" style="margin-left:7px;" onChange="change_dsl_transmode(this);">
													<option value="atm" <% nvram_match("dsltmp_transmode", "atm", "selected"); %>>ADSL WAN (ATM)</option>
													<option value="ptm" <% nvram_match("dsltmp_transmode", "ptm", "selected"); %>>VDSL WAN (PTM)</option>
												</select>
												<span id="active_dslmode"></span>
											</td>
										</tr>
									</table>

									<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable" id="DSL_WAN_table">
										<thead>
										<tr>
											<td colspan="8"><#DSL_multiserv_summary#></td>
										</tr>
										</thead>
											<tr>
												<th style="width:10%;"><center>Index</center></th>
												<th style="width:20%;"><center>802.1Q Enable</center></th>
												<th style="width:10%;"><center>VLAN ID</center></th>
												<th style="width:20%;"><center><#IPConnection_VServerProto_itemname#></center></th>
												<th style="width:10%;"><center><#Internet#></center></th>
												<th style="width:10%;"><center><#menu_dsl_iptv#></center></th>
												<th style="width:10%;"><center><#PVC_edit#></center></th>
												<th style="width:10%;" id="DSL_WAN_add_del"><center><#CTL_del#></center></th>
											</tr>
									</table>

									<table id="dslSettings" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
										<thead>
										<tr>
											<td colspan="2"><#pvccfg_PVC_Setting#></td>
										</tr>
										</thead>
										<tr>
											<th>PVC</th>
											<td><span id="pvc_sel" style="color:white;"></span></td>
										</tr>
										<tr>
											<th><#Layer3Forwarding_x_ConnectionType_itemname#></th>
											<td align="left">
												<select class="input_option" name="dsl_proto" onchange="change_dsl_type(this.value);fixed_change_dsl_type(this.value);">
													<option value="dhcp" <% nvram_match("dsl_proto", "dhcp", "selected"); %>><#BOP_ctype_title1#></option>
													<option value="static" <% nvram_match("dsl_proto", "static", "selected"); %>><#BOP_ctype_title5#></option>
													<option value="pppoe" <% nvram_match("dsl_proto", "pppoe", "selected"); %>>PPPoE</option>
													<option value="bridge" <% nvram_match("dsl_proto", "bridge", "selected"); %>>Bridge</option>
												</select>
											</td>
										</tr>
										<tr>
											<th><#Enable_WAN#></th>
											<td>
												<input type="radio" name="dsl_link_enable" class="input" value="1" <% nvram_match("dsl_link_enable", "1", "checked"); %>><#checkbox_Yes#>
												<input type="radio" name="dsl_link_enable" class="input" value="0" <% nvram_match("dsl_link_enable", "0", "checked"); %>><#checkbox_No#>
											</td>
										</tr>
										<tr>
											<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,22);"><#Enable_NAT#></a></th>
											<td>
												<input type="radio" name="dsl_nat" class="input" value="1" <% nvram_match("dsl_nat", "1", "checked"); %>><#checkbox_Yes#>
												<input type="radio" name="dsl_nat" class="input" value="0" <% nvram_match("dsl_nat", "0", "checked"); %>><#checkbox_No#>
											</td>
										</tr>
										<tr>
											<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,23);"><#BasicConfig_EnableMediaServer_itemname#></a></th>
											<td>
												<input type="radio" name="dsl_upnp_enable" class="input" value="1" onclick="return change_common_radio(this, 'LANHostConfig', 'dsl_upnp_enable', '1')" <% nvram_match("dsl_upnp_enable", "1", "checked"); %>><#checkbox_Yes#>
												<input type="radio" name="dsl_upnp_enable" class="input" value="0" onclick="return change_common_radio(this, 'LANHostConfig', 'dsl_upnp_enable', '0')" <% nvram_match("dsl_upnp_enable", "0", "checked"); %>><#checkbox_No#>
											</td>
										</tr>
									</table>

									<table id="dot1q_setting" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
										<thead>
										<tr>
											<td colspan="2">802.1Q</td>
										</tr>
										</thead>
										<tr>
											<th><#WLANConfig11b_WirelessCtrl_button1name#></th>
											<td>
												<input type="radio" name="dsl_dot1q" class="input" value="1" onclick="change_dsl_dhcp_enable();" <% nvram_match("dsl_dot1q", "1", "checked"); %>><#checkbox_Yes#>
												<input type="radio" name="dsl_dot1q" class="input" value="0" onclick="change_dsl_dhcp_enable();" <% nvram_match("dsl_dot1q", "0", "checked"); %>><#checkbox_No#>
											</td>
										</tr>
										<tr>
											<th>VLAN ID</th>
											<td>
												<input type="text" name="dsl_vid" maxlength="4" class="input_6_table" value="<% nvram_get("dsl_vid"); %>" onKeyPress="return validator.isNumber(this,event);" autocorrect="off" autocapitalize="off"> 0 - 4095
											</td>
										</tr>
										<tr>
											<th>802.1P</th>
											<td>
												<input type="text" name="dsl_dot1p" maxlength="4" class="input_6_table" value="<% nvram_get("dsl_dot1p"); %>" onKeyPress="return validator.isNumber(this,event);"> 0 - 7
											</td>
										</tr>
									<table>

									<table id="IPsetting" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
										<thead>
										<tr>
											<td colspan="2"><#IPConnection_ExternalIPAddress_sectionname#></td>
										</tr>
										</thead>

										<tr>
											<th><#Layer3Forwarding_x_DHCPClient_itemname#></th>
											<td>
												<input type="radio" name="dsl_DHCPClient_x" class="input" value="1" onclick="change_dsl_dhcp_enable();" <% nvram_match("dsl_DHCPClient", "1", "checked"); %>><#checkbox_Yes#>
												<input type="radio" name="dsl_DHCPClient_x" class="input" value="0" onclick="change_dsl_dhcp_enable();" <% nvram_match("dsl_DHCPClient", "0", "checked"); %>><#checkbox_No#>
											</td>
										</tr>

										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,1);"><#IPConnection_ExternalIPAddress_itemname#></a>
											</th>
											<td>
												<input type="text" name="dsl_ipaddr" maxlength="15" class="input_15_table" value="<% nvram_get("dsl_ipaddr"); %>" onKeyPress="return validator.isIPAddr(this,event);" autocorrect="off" autocapitalize="off">
											</td>
										</tr>

										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,2);"><#IPConnection_x_ExternalSubnetMask_itemname#></a>
											</th>
											<td>
												<input type="text" name="dsl_netmask" maxlength="15" class="input_15_table" value="<% nvram_get("dsl_netmask"); %>" onKeyPress="return validator.isIPAddr(this,event);" autocorrect="off" autocapitalize="off">
											</td>
										</tr>

										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,3);"><#IPConnection_x_ExternalGateway_itemname#></a>
											</th>
											<td>
												<input type="text" name="dsl_gateway" maxlength="15" class="input_15_table" value="<% nvram_get("dsl_gateway"); %>" onKeyPress="return validator.isIPAddr(this,event);" autocorrect="off" autocapitalize="off">
											</td>
										</tr>
									</table>

									<table id="DNSsetting" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
										<thead>
										<tr>
											<td colspan="2"><#IPConnection_x_DNSServerEnable_sectionname#></td>
										</tr>
										</thead>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,12);"><#IPConnection_x_DNSServerEnable_itemname#></a>
											</th>
											<td>
												<input type="radio" name="dsl_dnsenable" class="input" value="1" onclick="change_dsl_dns_enable()" <% nvram_match("dsl_dnsenable", "1", "checked"); %> /><#checkbox_Yes#>
												<input type="radio" name="dsl_dnsenable" class="input" value="0" onclick="change_dsl_dns_enable()" <% nvram_match("dsl_dnsenable", "0", "checked"); %> /><#checkbox_No#>
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,13);"><#IPConnection_x_DNSServer1_itemname#></a>
											</th>
											<td>
												<input type="text" maxlength="15" class="input_15_table" name="dsl_dns1" value="<% nvram_get("dsl_dns1"); %>" onkeypress="return validator.isIPAddr(this,event)" autocorrect="off" autocapitalize="off"/>
												<img id="dns_pull_arrow1" class="dns_pull_arrow" src="/images/arrow-down.gif" onclick="pullDNSList(this);">
												<div id="dns_server_list1" class="dns_server_list_dropdown"></div>
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,14);"><#IPConnection_x_DNSServer2_itemname#></a>
											</th>
											<td>
												<input type="text" maxlength="15" class="input_15_table" name="dsl_dns2" value="<% nvram_get("dsl_dns2"); %>" onkeypress="return validator.isIPAddr(this,event)" autocorrect="off" autocapitalize="off"/>
												<img id="dns_pull_arrow2" class="dns_pull_arrow" src="/images/arrow-down.gif" onclick="pullDNSList(this);">
												<div id="dns_server_list2" class="dns_server_list_dropdown"></div>
											</td>
										</tr>
										<tr style="display:none">
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,35);"><#WAN_DNS_Privacy#></a>
											</th>
											<td align="left">
												<select id="dnspriv_enable" class="input_option" name="dnspriv_enable" onChange="change_dnspriv_enable(this.value);">
												<option value="0" <% nvram_match("dnspriv_enable", "0", "selected"); %>><#wl_securitylevel_0#></option>
												<option value="1" <% nvram_match("dnspriv_enable", "1", "selected"); %>>DNS-over-TLS (DoT)</option>
												<!--option value="2" <% nvram_match("dnspriv_enable", "2", "selected"); %>>DNS-over-HTTPS (DoH)</option-->
												<!--option value="3" <% nvram_match("dnspriv_enable", "3", "selected"); %>>DNS-over-TLS/HTTPS (DoT+DoH)</option-->
												</select>
												<div id="yadns_hint_dnspriv" style="display:none;"></div>
											</td>
										</tr>
										<tr style="display:none">
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,36);"><#WAN_DNS_over_TLS#></a>
											</th>
											<td>
												<input type="radio" name="dnspriv_profile" class="input" value="1" onclick="return change_common_radio(this, 'IPConnection', 'dnspriv_profile', 1)" <% nvram_match("dnspriv_profile", "1", "checked"); %> /><#WAN_DNS_over_TLS_Strict#>
												<input type="radio" name="dnspriv_profile" class="input" value="0" onclick="return change_common_radio(this, 'IPConnection', 'dnspriv_profile', 0)" <% nvram_match("dnspriv_profile", "0", "checked"); %> /><#WAN_DNS_over_TLS_Opportunistic#>
											</td>
										</tr>
									</table>

									<table id="DNSPrivacy" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable_table" style="display:none">
									<thead>
			  						<tr>
										<td colspan="5"><#WAN_DNS_over_TLS_server#>&nbsp;(<#List_limit#>&nbsp;8)</td>
			  						</tr>
									</thead>
									<tr>
										<th><a href="javascript:void(0);" onClick="openHint(7,37);"><div class="table_text"><div class="table_text"><#IPConnection_ExternalIPAddress_itemname#></div></a></th>
										<th><a href="javascript:void(0);" onClick="openHint(7,38);"><div class="table_text"><div class="table_text"><#WAN_DNS_over_TLS_server_port#></div></a></th>
										<th><a href="javascript:void(0);" onClick="openHint(7,39);"><div class="table_text"><div class="table_text"><#WAN_DNS_over_TLS_server_name#></div></a></th>
										<th><a href="javascript:void(0);" onClick="openHint(7,40);"><div class="table_text"><div class="table_text"><#WAN_DNS_over_TLS_server_SPKI#></div></a></th>
										<th><#list_add_delete#></th>
									</tr>
									<!-- server info -->
									<tr>
										<td width="27%"><input type="text" class="input_20_table" maxlength="64" name="dnspriv_server_0" onKeyPress="" autocorrect="off" autocapitalize="off"></td>
										<td width="10%"><input type="text" class="input_6_table" maxlength="5" name="dnspriv_port_0" onKeyPress="return validator.isNumber(this,event)" autocorrect="off" autocapitalize="off"></td>
										<td width="27%"><input type="text" class="input_20_table" maxlength="64" name="dnspriv_hostname_0" onKeyPress="" autocorrect="off" autocapitalize="off"></td>
										<td width="27%"><input type="text" class="input_20_table" maxlength="64" name="dnspriv_spkipin_0" onKeyPress="" autocorrect="off" autocapitalize="off"></td>
										<td width="9%"><div><input type="button" class="add_btn" onClick="addRow_Group(8);" value=""></div></td>
									</tr>
									</table>
									<!-- server block -->
									<div id="dnspriv_rulelist_Block"></div>

									<table id="DHCP_option" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
										<thead>
											<tr><td colspan="2"><#ipv6_6rd_dhcp_option#></td></tr>
										</thead>
										<tr>
											<th width="40%"><#DHCPoption_Class#> (<#NetworkTools_option#> 60):</th>
											<td>
												<input type="text" name="dsl_dhcp_vendorid" class="input_25_table" value="<% nvram_get("dsl_dhcp_vendorid"); %>" maxlength="126" autocapitalization="off" autocomplete="off">
											</td>
										</tr>
										<tr>
											<th width="40%"><#DHCPoption_Client#> (<#NetworkTools_option#> 61):</th>
											<td>
												<input type="checkbox" id="tmp_dhcp_clientid_type" name="tmp_dhcp_clientid_type" onclick="showDiableDHCPclientID(this);" <% nvram_match("dsl_dhcp_clientid_type", "1", "checked"); %>>IAID/DUID<br>
												<input type="text" name="dsl_dhcp_clientid" class="input_25_table" value="<% nvram_get("dsl_dhcp_clientid"); %>" maxlength="126" autocapitalization="off" autocomplete="off">
											</td>
										</tr>
									</table>

									<table id="PPPsetting" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
										<thead>
										<tr>
											<td colspan="2"><#PPPConnection_UserName_sectionname#></td>
										</tr>
										</thead>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,4);"><#Username#></a>
											</th>
											<td>
												<input type="text" maxlength="64" class="input_32_table" name="dsl_pppoe_username" value="<% nvram_get("dsl_pppoe_username"); %>" onkeypress="return validator.isString(this, event)" onblur="" autocomplete="off" autocorrect="off" autocapitalize="off">
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,5);"><#PPPConnection_Password_itemname#></a>
											</th>
											<td>
												<div style="margin-top:2px;"><input type="password" maxlength="64" class="input_32_table" id="dsl_pppoe_passwd" name="dsl_pppoe_passwd" value="<% nvram_get("dsl_pppoe_passwd"); %>" autocomplete="off" autocorrect="off" autocapitalize="off"></div>
												<div style="margin-top:1px;"><input type="checkbox" name="show_pass_1" onclick="pass_checked(document.form.dsl_pppoe_passwd);"><#QIS_show_pass#></div>
											</td>
										</tr>
										<tr>
											<th><#WAN_PPP_AuthText#></th>
											<td align="left">
												<select class="input_option" name="dsl_pppoe_auth">
													<option value="" <% nvram_match("dsl_pppoe_auth", "", "selected"); %>>AUTO</option>
													<option value="pap" <% nvram_match("dsl_pppoe_auth", "pap", "selected"); %>>PAP</option>
													<option value="chap" <% nvram_match("dsl_pppoe_auth", "chap", "selected"); %>>CHAP</option>
												</select>
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,6);"><#PPPConnection_IdleDisconnectTime_itemname#></a>
											</th>
											<td>
												<input type="text" maxlength="10" class="input_12_table" name="dsl_pppoe_idletime" value="<% nvram_get("dsl_pppoe_idletime"); %>" onkeypress="return validator.isNumber(this,event)" autocorrect="off" autocapitalize="off"/>&nbsp<#Second#>
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,7);">PPP <#PPPConnection_x_PPPoEMTU_itemname#></a>
											</th>
											<td>
												<input type="text" maxlength="5" name="dsl_pppoe_mtu" class="input_6_table" value="<% nvram_get("dsl_pppoe_mtu"); %>" onKeyPress="return validator.isNumber(this,event);" autocorrect="off" autocapitalize="off"/>&nbsp;128 - 1492
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,8);">PPP <#PPPConnection_x_PPPoEMRU_itemname#></a>
											</th>
											<td>
												<input type="text" maxlength="5" name="dsl_pppoe_mru" class="input_6_table" value="<% nvram_get("dsl_pppoe_mru"); %>" onKeyPress="return validator.isNumber(this,event);" autocorrect="off" autocapitalize="off"/>&nbsp;128 - 1492
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,9);"><#PPPConnection_x_ServiceName_itemname#></a>
											</th>
											<td>
												<input type="text" maxlength="32" class="input_32_table" name="dsl_pppoe_service" value="<% nvram_get("dsl_pppoe_service"); %>" onkeypress="return validator.isString(this, event)" onblur="" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,10);"><#PPPConnection_x_AccessConcentrator_itemname#></a>
											</th>
											<td>
												<input type="text" maxlength="32" class="input_32_table" name="dsl_pppoe_ac" value="<% nvram_get("dsl_pppoe_ac"); %>" onkeypress="return validator.isString(this, event)" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,18);">Host-Uniq (<#Hexadecimal#>)</a>
											</th>
											<td align="left">
												<input type="text" maxlength="256" class="input_32_table" name="dsl_pppoe_hostuniq" value="<% nvram_get("dsl_pppoe_hostuniq"); %>" onkeypress="return validator.isString(this, event);" autocorrect="off" autocapitalize="off"/>
											</td>
										</tr>
										<tr>
											<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,31);"><#PPPConnection_x_InternetDetection_itemname#></a></th>
											<td>
												<select name="wan_ppp_echo" class="input_option" onChange="ppp_echo_control();">
												<option value="0" <% nvram_match("wan_ppp_echo", "0","selected"); %>><#btn_disable#></option>
												<option value="1" <% nvram_match("wan_ppp_echo", "1","selected"); %>>PPP Echo</option>
												<option value="2" <% nvram_match("wan_ppp_echo", "2","selected"); %>>DNS Probe</option>
												</select>
											</td>
										</tr>
										<tr>
											<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,32);"><#PPPConnection_x_PPPEcho_Interval#></a></th>
											<td><input type="text" maxlength="6" class="input_6_table" name="wan_ppp_echo_interval" value="<% nvram_get("wan_ppp_echo_interval"); %>" onkeypress="return validator.isNumber(this, event)" autocorrect="off" autocapitalize="off"/></td>
										</tr>
										<tr>
											<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,33);"><#PPPConnection_x_PPPEcho_Max_Failure#></a></th>
											<td><input type="text" maxlength="6" class="input_6_table" name="wan_ppp_echo_failure" value="<% nvram_get("wan_ppp_echo_failure"); %>" onkeypress="return validator.isNumber(this,event);" autocorrect="off" autocapitalize="off"/></td>
										</tr>
										<!--tr>
											<th><a class="hintstyle" href="javascript:void(0);">DNS Probe Timeout</a></th><!--untranslated--\>
											<td><input type="text" maxlength="6" class="input_6_table" name="dns_probe_timeout" value="<% nvram_get("dns_probe_timeout"); %>" onkeypress="return validator.isNumber(this, event)" autocorrect="off" autocapitalize="off"/></td>
										</tr-->
										<tr>
											<th><a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,34);">DNS Probe Max Failures</a></th><!--untranslated-->
											<td><input type="text" maxlength="6" class="input_6_table" name="dns_delay_round" value="<% nvram_get("dns_delay_round"); %>" onkeypress="return validator.isNumber(this,event);" autocorrect="off" autocapitalize="off"/></td>
										</tr>
										<!-- 2008.03 James. patch for Oleg's patch. { -->
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,18);"><#PPPConnection_x_AdditionalOptions_itemname#></a>
											</th>
											<td>
												<input type="text" name="dsl_pppoe_options" value="<% nvram_get("dsl_pppoe_options"); %>" class="input_32_table" maxlength="255" onKeyPress="return validator.isString(this, event)" onBlur="validator.string(this)" autocorrect="off" autocapitalize="off">
											</td>
										</tr>
										<!-- 2008.03 James. patch for Oleg's patch. } -->
									</table>

									<table id="vpn_server" width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
										<thead>
										<tr>
											<td colspan="2"><#PPPConnection_x_HostNameForISP_sectionname#></td>
										</tr>
										</thead>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,16);"><#PPPConnection_x_MacAddressForISP_itemname#></a>
											</th>
											<td>
												<input type="text" name="dsl_hwaddr" class="input_20_table" maxlength="17" value="<% nvram_get("dsl_hwaddr"); %>" onKeyPress="return validator.isHWAddr(this,event)" autocorrect="off" autocapitalize="off">
												<input type="button" class="button_gen" onclick="showMAC();" value="<#BOP_isp_MACclone#>">
											</td>
										</tr>
										<tr>
											<th>
												<a class="hintstyle" href="javascript:void(0);" onClick="openHint(7,30);"><#DHCP_query_freq#></a>
											</th>
											<td>
												<select name="dsl_dhcp_qry" class="input_option">
													<option value="0" <% nvram_match(" dsl_dhcp_qry", "0","selected"); %>><#DHCPnormal#></option>
													<option value="1" <% nvram_match(" dsl_dhcp_qry", "1","selected"); %>><#DHCPaggressive#></option>
													<option value="2" <% nvram_match(" dsl_dhcp_qry", "2","selected"); %>><#Continuous_Mode#></option>
												</select>
											</td>
										</tr>
										<tr>
											<th>
												<#PPPConnection_x_PPPoEMTU_itemname#>
											</th>
											<td>
												<input type="text" maxlength="5" name="dsl_mtu" class="input_6_table" value="<% nvram_get("dsl_mtu"); %>" onKeyPress="return validator.isNumber(this,event);" autocorrect="off" autocapitalize="off"/>&nbsp;1280 - 1500
											</td>
										</tr>
									</table>

									<div id="btn_apply" class="apply_gen" style="height:auto">
										<input class="button_gen" onclick="exit_to_main();" type="button" value="<#CTL_Cancel#>">
										<input class="button_gen" onclick="applyRule();" type="button" value="<#CTL_ok#>"/>
									</div>

								</td>
							</tr>
							</tbody>
						</table>
					</td>
				</form>
			</table>
		</td>
		<!--===================================Ending of Main Content===========================================-->
		<td width="10" align="center" valign="top">&nbsp;</td>
	</tr>
</table>
<div id="footer"></div>
<form method="post" name="chgpvc" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="flag" value="chg_pvc">
<input type="hidden" name="action_script" value="">
<input type="hidden" name="action_wait" value="">
<input type="hidden" name="current_page" value="Advanced_VDSL_Content.asp">
<input type="hidden" name="dsl_unit" value="8">
<input type="hidden" name="dsl_subunit" value="">
</form>
</body>
</html>
