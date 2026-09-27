<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<html xmlns:v>
<head>
<meta http-equiv="X-UA-Compatible" content="IE=Edge"/>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<title><#Web_Title#> - DNS-based Filtering</title>
<link rel="shortcut icon" href="images/favicon.png">
<link rel="icon" href="images/favicon.png">
<link rel="stylesheet" type="text/css" href="ParentalControl.css">
<link rel="stylesheet" type="text/css" href="index_style.css"> 
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="usp_style.css">
<link rel="stylesheet" type="text/css" href="/device-map/device-map.css">
<script type="text/javascript" src="/js/jquery.js"></script>
<script type="text/javascript" src="/js/httpApi.js"></script>
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/help.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/client_function.js"></script>
<script type="text/javascript" src="/validator.js"></script>
<script type="text/javascript" src="/switcherplugin/jquery.iphone-switch.js"></script>
<script>

var dnsfilter_rulelist_array = new Array();

var modes_array = [
	[ "0", "<#No_Filtering#>" ],
	[ "11", "<#Device_type_02_RT#>" ],
	[ "14", "<#CleanBrowsing_Security#>" ],
	[ "15", "<#CleanBrowsing_Adult#>" ],
	[ "16", "<#CleanBrowsing_Family#>" ],
	[ "12", "<#Comodo_DNS#>" ],
	[ "1",  "<#OpenDNS_Home#>" ],
	[ "7",  "<#OpenDNS_Family#>" ],
	[ "13", "<#Quad9#>" ],
	[ "5",  "<#Yandex_Safe#>" ],
	[ "6",  "<#Yandex_Family#>" ],
	[ "17", "Cloudflare Safe" ],
	[ "18", "Cloudflare Family" ],
	[ "19", "AdGuard Ad block" ],
	[ "20", "AdGuard Family" ],
	[ "8",  "<#Custom#> 1" ],
	[ "9",  "<#Custom#> 2" ],
	[ "10", "<#Custom#> 3" ],
];

function initial(){
	show_menu();
	show_footer();

	var dnsfilter_rulelist = decodeURIComponent(httpApi.nvramCharToAscii(["dnsfilter_rulelist"], true).dnsfilter_rulelist);
	if (isSupport("hnd")) {
		var hnd_count = 5;
		for(var i = 1; i <= hnd_count; i +=1)
			dnsfilter_rulelist += decodeURIComponent(httpApi.nvramCharToAscii(["dnsfilter_rulelist" + i + ""], true)["dnsfilter_rulelist" + i + ""]);
	}
	var dnsfilter_row = dnsfilter_rulelist.split("<");
	for(var i = 0; i < dnsfilter_row.length; i += 1) {
		if(dnsfilter_row[i] != ""){
			var dnsfilter_col = dnsfilter_row[i].split(">");
			var clientMac = dnsfilter_col[1];
			var filterMode = dnsfilter_col[2];
			dnsfilter_rulelist_array[clientMac] = filterMode;
		}
	}

	show_dnsfilter_list();
	showDropdownClientList('setClientmac', 'mac', 'all', 'ClientList_Block_PC', 'pull_arrow', 'all');

	showhide_settings(document.form.dnsfilter_enable_x.value);

	gen_modeselect("dnsfilter_mode", "<% nvram_get("dnsfilter_mode"); %>", "");
	gen_modeselect("client_modesel", "-1", "");
}

/*------------ Mouse event of fake LAN IP select menu {-----------------*/
function setClientmac(macaddr){
	document.form.rule_mac.value = macaddr;
	hideClients_Block();
}

function pullLANIPList(obj){
	var element = document.getElementById('ClientList_Block_PC');
	var isMenuopen = element.offsetWidth > 0 || element.offsetHeight > 0;
	if(isMenuopen == 0){
		obj.src = "/images/unfold_less.svg"
		element.style.display = 'block';
		document.form.rule_mac.focus();
	}
	else
		hideClients_Block();
}

function hideClients_Block(){
	document.getElementById("pull_arrow").src = "/images/unfold_more.svg";
	document.getElementById('ClientList_Block_PC').style.display = 'none';
}

function gen_modeselect(name, value, onchange){
	var code = "";
	var i;

	if ((name == "client_modesel") || (name == "dnsfilter_mode")) {
		for(i = 0; i < modes_array.length; i++){
			add_option(document.getElementById(name),
			           modes_array[i][1], modes_array[i][0],
			           (value == modes_array[i][0]));
	        }
	} else {
		code = '<select class="input_option" name="'+name+'" value="'+value+'" onchange="'+onchange+'">';
		for(i = 0; i < modes_array.length; i++){
			var itemval = modes_array[i][0];
			code +='<option value="' + itemval + '"' + (value == itemval ? "selected" : "") +'>' + modes_array[i][1] + '</option>';
		}
		code += '</select>';
	}

	return code;
}

function show_dnsfilter_list(){
	var code = "";
	var clientListEventData = [];

	code = '<table width="100%" border="1" cellspacing="0" cellpadding="4" align="center" class="list_table" id="clientTable">';
	if(Object.keys(dnsfilter_rulelist_array).length == 0)
		code += '<tr><td style="color:#FFCC00;" colspan="3"><#IPConnection_VSList_Norule#></td></tr>';
	else{
		//user icon
		var userIconBase64 = "NoIcon";
		var clientName, deviceType, deviceVendor;
		Object.keys(dnsfilter_rulelist_array).forEach(function(key) {
			var clientMac = key.toUpperCase();
			var clientIconID = "clientIcon_" + clientMac.replace(/\:/g, "");
			var rule_mode = dnsfilter_rulelist_array[key];

			if(clientList[clientMac]) {
				clientName = (clientList[clientMac].nickName == "") ? clientList[clientMac].name : clientList[clientMac].nickName;
				deviceType = clientList[clientMac].type;
				deviceVendor = clientList[clientMac].vendor;
			}
			else {
				clientName = "New device";
				deviceType = 0;
				deviceVendor = "";
			}
			code +='<tr>';
			code +='<td width="50%" title="'+clientName+'">';

			code += '<table style="width:100%;"><tr><td style="width:40%;height:56px;border:0px;float:right;">';
			if(clientList[clientMac] == undefined) {
				code += '<div id="' + clientIconID + '" class="clientIcon type0"></div>';
			}
			else {
				if(usericon_support) {
					userIconBase64 = getUploadIcon(clientMac.replace(/\:/g, ""));
				}
				if(userIconBase64 != "NoIcon") {
                    if(clientList[clientMac].isUserUplaodImg){
                        code += '<div id="' + clientIconID + '" class="clientIcon"><img class="imgUserIcon_card" src="' + userIconBase64 + '"></div>';
                    }else{
                        code += '<div id="' + clientIconID + '" class="clientIcon"><i class="type" style="--svg:url(' + userIconBase64 + ')"></i></div>';
                    }
                }
                else if(deviceType != "0" || deviceVendor == "") {
                    code += '<div id="' + clientIconID + '" class="clientIcon"><i class="type'+deviceType+'"></i></div>';
                }
                else if(deviceVendor != "" ) {
                    var vendorIconClassName = getVendorIconClassName(deviceVendor.toLowerCase());
                    if(vendorIconClassName != "" && !downsize_4m_support) {
                        code += '<div id="' + clientIconID + '" class="clientIcon"><i class="vendor-icon '+ vendorIconClassName +'"></i></div>';
                    }
                    else {
                        code += '<div id="' + clientIconID + '" class="clientIcon"><i class="type' + deviceType + '"></i></div>';
                    }
                }
			}
			code += '</td><td style="width:60%;text-align:left;border:0;">';
			code += '<div>' + clientName + '</div>';
			code += '<div>' + clientMac + '</div>';
			code += '</td></tr></table>';
			code += '</td>';

			code +='<td width="35%">'+gen_modeselect("rule_mode"+i, rule_mode, "changeRow_main(this, \'" + clientMac + "\');")+'</td>';
			code +='<td width="15%"><input class="remove_btn" onclick="deleteRow_main(this, \'' + clientMac + '\');" value=""/></td></tr>';
			if(validator.mac_addr(clientMac))
				clientListEventData.push({"mac" : clientMac, "name" : "", "ip" : "", "callBack" : "DNSFilter"});
		});
	}

	code += '</table>';
	document.getElementById("mainTable_Block").innerHTML = code;
	for(var i = 0; i < clientListEventData.length; i += 1) {
		var clientIconID = "clientIcon_" + clientListEventData[i].mac.replace(/\:/g, "");
		var clientIconObj = $("#mainTable_Block").children("#clientTable").find("#" + clientIconID + "")[0];
		var paramData = JSON.parse(JSON.stringify(clientListEventData[i]));
		paramData["obj"] = clientIconObj;
		$("#mainTable_Block").children("#clientTable").find("#" + clientIconID + "").click(paramData, popClientListEditTable);
	}
}

function applyRule(){
	var dnsfilter_rule_list = "";
	Object.keys(dnsfilter_rulelist_array).forEach(function(key) {
		var clientMac = key.toUpperCase();
		var mode = dnsfilter_rulelist_array[clientMac];
		dnsfilter_rule_list += "<>";
		dnsfilter_rule_list += clientMac;
		dnsfilter_rule_list += ">";
		dnsfilter_rule_list += mode;
	});

	if (isSupport("hnd"))
		split_clientlist(dnsfilter_rule_list);
	else
		document.form.dnsfilter_rulelist.value = dnsfilter_rule_list;

	showLoading();
	document.form.submit();
}

function split_clientlist(clientlist){
	var counter = 0;

	document.form.dnsfilter_rulelist.value = clientlist.substring(counter, (counter+=255))
	document.form.dnsfilter_rulelist1.value = clientlist.substring(counter, (counter+=255));
	document.form.dnsfilter_rulelist2.value = clientlist.substring(counter, (counter+=255));
	document.form.dnsfilter_rulelist3.value = clientlist.substring(counter, (counter+=255));
	document.form.dnsfilter_rulelist4.value = clientlist.substring(counter, (counter+=255));
	document.form.dnsfilter_rulelist5.value = clientlist.substring(counter, (counter+=255));
}

function check_macaddr(obj,flag){ //control hint of input mac address
	if(flag == 1){
		var childsel=document.createElement("div");
		childsel.setAttribute("id","check_mac");
		childsel.style.color="#FFCC00";
		obj.parentNode.appendChild(childsel);
		document.getElementById("check_mac").innerHTML="<#LANHostConfig_ManualDHCPMacaddr_itemdesc#>";
		document.getElementById("check_mac").style.display = "";
		return false;
	}else if(flag == 2){
		var childsel=document.createElement("div");
		childsel.setAttribute("id","check_mac");
		childsel.style.color="#FFCC00";
		obj.parentNode.appendChild(childsel);
		document.getElementById("check_mac").innerHTML="<#IPConnection_x_illegal_mac#>";
		document.getElementById("check_mac").style.display = "";
		return false;
	}else{
		document.getElementById("check_mac") ? document.getElementById("check_mac").style.display="none" : true;
		return true;
	}
}

function addRow_main(upper){
	var rule_num = Object.keys(dnsfilter_rulelist_array).length;
	if(rule_num >= upper){
		alert("<#JS_itemlimit1#> " + upper + " <#JS_itemlimit2#>");
		return false;
	}

	if(document.form.rule_mac.value == ""){
		alert("<#JS_fieldblank#>");
		document.form.rule_mac.focus();
		return false;
	}

	if(!check_macaddr(document.form.rule_mac, check_hwaddr_flag(document.form.rule_mac,'inner'))){
		document.form.rule_mac.focus();
		document.form.rule_mac.select();
		return false;
	}

	if(dnsfilter_rulelist_array[document.form.rule_mac.value.toUpperCase()] != null){
		alert("<#JS_duplicate#>");
		document.form.rule_mac.focus();
		document.form.rule_mac.select();
		return false;
	}
	dnsfilter_rulelist_array[document.form.rule_mac.value.toUpperCase()] = document.form.rule_mode.value;

	document.form.rule_mac.value = "";
	document.form.rule_mode.value = 0;
	show_dnsfilter_list();
}

function deleteRow_main(r, clientMac){
	var delIndex = r.parentNode.parentNode.rowIndex;
	delete dnsfilter_rulelist_array[clientMac];
	document.getElementById('clientTable').deleteRow(delIndex);
	if(Object.keys(dnsfilter_rulelist_array).length == 0)
		show_dnsfilter_list();
}

function changeRow_main(r, clientMac){
	dnsfilter_rulelist_array[clientMac] = $(r).val();
}

function showhide_settings(state) {
	showhide("dnsfilter_mode_tr", state);
	showhide("dnsfilter_custom1", state);
	showhide("dnsfilter_custom2", state);
	showhide("dnsfilter_custom3", state);
	showhide("mainTable_Table", state);
	showhide("mainTable_Block", state);
}

</script>
</head>

<body onload="initial();" onunload="unload_body();" class="bg">
<div id="TopBanner"></div>
<div id="Loading" class="popup_bg"></div>

<iframe name="hidden_frame" id="hidden_frame" width="0" height="0" frameborder="0"></iframe>
<form method="post" name="form" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="productid" value="<% nvram_get("productid"); %>">
<input type="hidden" name="current_page" value="DNSFilter.asp">
<input type="hidden" name="next_page" value="">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="action_wait" value="5">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="restart_dnsfilter">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="dnsfilter_enable_x" value="<% nvram_get("dnsfilter_enable_x"); %>">
<input type="hidden" name="dnsfilter_rulelist" value="">
<input type="hidden" name="dnsfilter_rulelist1" value="">
<input type="hidden" name="dnsfilter_rulelist2" value="">
<input type="hidden" name="dnsfilter_rulelist3" value="">
<input type="hidden" name="dnsfilter_rulelist4" value="">
<input type="hidden" name="dnsfilter_rulelist5" value="">

<table class="content" align="center" cellpadding="0" cellspacing="0" >
	<tr>
		<td width="17">&nbsp;</td>
		<td valign="top" width="202">
			<div  id="mainMenu"></div>
			<div  id="subMenu"></div>
		</td>
		<td valign="top">
			<div id="tabMenu" class="submenuBlock"></div>
			<!--===================================Beginning of Main Content===========================================-->
			<table width="98%" border="0" align="left" cellpadding="0" cellspacing="0" >
				<tr>
					<td valign="top" >
						<table width="730px" border="0" cellpadding="4" cellspacing="0" class="FormTitle" id="FormTitle">
							<tbody>
								<tr>
									<td bgcolor="#4D595D" valign="top">
										<div>&nbsp;</div>
										<div class="formfonttitle"><#DNSFilter#></div>
										<div style="margin:10px 0 10px 5px;" class="splitLine"></div>
										<div id="dnsfilter_desc" style="margin-bottom:10px;">
											<table width="700px" style="margin-left:25px;">
												<tr>
													<td>
														<img id="guest_image" src="/images/New_ui/DnsFiltering.png">
													</td>
													<td>&nbsp;&nbsp;</td>
													<td style="font-style: italic;font-size: 14px;">
														<div>
															<p><#DNSFilter_Desc#></p>
															<ul>
																<li>
																	<a target="_blank" style="font-weight: bolder; cursor:pointer;text-decoration: underline;" href="https://cleanbrowsing.org/"><#CleanBrowsing#></a>
																	<ul>
																		<li><#CleanBrowsing_Security_Desc#></li>
																		<li><#CleanBrowsing_Adult_Desc#></li>
																		<li><#CleanBrowsing_Family_Desc#></li>
																	</ul>
																</li>
																<li>
																	<a target="_blank" style="font-weight: bolder; cursor:pointer;text-decoration: underline;" href="https://www.comodo.com/secure-dns/"><#Comodo_DNS#></a>
																	<ul>
																		<li><#Protects_malicious_content#></li>
																	</ul>
																</li>
																<li>
																	<a target="_blank" style="font-weight: bolder; cursor:pointer;text-decoration: underline;" href="https://www.opendns.com/home-internet-security/"><#OpenDNS#></a>
																	<ul>
																		<li><#OpenDNS_Home_Desc#></li>
																		<li><#OpenDNS_Family_Desc#></li>
																	</ul>
																</li>
																<li>
																	<a target="_blank" style="font-weight: bolder; cursor:pointer;text-decoration: underline;" href="https://www.quad9.net"><#Quad9#></a>
																	<ul>
																		<li><#Protects_malicious_content#></li>
																	</ul>
																</li>
																<li>
																	<a target="_blank" style="font-weight: bolder; cursor:pointer;text-decoration: underline;" href="https://dns.yandex.com">
																		<#YandexDNS#>
																	</a>
																	<ul>
																		<li><#Yandex_Safe_Desc#></li>
																		<li><#Yandex_Family_Desc#></li>
																	</ul>
																</li>
															</ul>
															<br>
															<#No_Filtering_Desc#>
														</div>
													</td>
												</tr>
											</table>
										</div>
										<!--=====Beginning of Main Content=====-->
										<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
											<tr>
												<th><#Enable_DNS_Filtering#></th>
												<td>
													<div align="center" class="left" style="width:94px; float:left; cursor:pointer;" id="radio_dnsfilter_enable"></div>
													<div class="iphone_switch_container" style="height:32px; width:74px; position: relative; overflow: hidden">
														<script type="text/javascript">
															$('#radio_dnsfilter_enable').iphoneSwitch(document.form.dnsfilter_enable_x.value,
																function(){
																	document.form.dnsfilter_enable_x.value = 1;
																	showhide_settings(1);
																},
																function(){
																	document.form.dnsfilter_enable_x.value = 0;
																	showhide_settings(0);
																},
																{
																	switch_on_container_path: '/switcherplugin/iphone_switch_container_off.png'
																});
														</script>
													</div>
												</td>
											</tr>
											<tr id="dnsfilter_mode_tr">
												<th><#Global_Filter_Mode#></th>
												<td>
													<select name="dnsfilter_mode" id="dnsfilter_mode" class="input_option"></select>
												</td>
											</tr>
											<tr id="dnsfilter_custom1">
												<th width="200"><#Custom_DNS_1#></th>
												<td>
													<input type="text" maxlength="15" class="input_15_table" name="dnsfilter_custom1" value="<% nvram_get("dnsfilter_custom1"); %>" onKeyPress="return validator.isIPAddr(this,event)">
												</td>
											</tr>
											<tr id="dnsfilter_custom2">
												<th width="200"><#Custom_DNS_2#></th>
												<td>
													<input type="text" maxlength="15" class="input_15_table" name="dnsfilter_custom2" value="<% nvram_get("dnsfilter_custom2"); %>" onKeyPress="return validator.isIPAddr(this,event)">
												</td>
											</tr>
											<tr id="dnsfilter_custom3">
												<th width="200"><#Custom_DNS_3#></th>
												<td>
													<input type="text" maxlength="15" class="input_15_table" name="dnsfilter_custom3" value="<% nvram_get("dnsfilter_custom3"); %>" onKeyPress="return validator.isIPAddr(this,event)">
												</td>
												</tr>
											</table>
										<table width="100%" border="1" cellspacing="0" cellpadding="4" align="center" class="FormTable_table" style="margin-top:8px;" id="mainTable_Table">
											<thead>
												<tr>
													<td colspan="3"><#ConnectedClient#>&nbsp;(<#List_limit#>&nbsp;64)</td>
												</tr>
											</thead>
											<tr>
												<th><#ParentalCtrl_hwaddr#></th>
												<th><#Filter_Mode#></th>
												<th><#list_add_delete#></th>
											</tr>
											<tr>
												<td width="50%">
												    <div class="clientlist_dropdown_main" style="width: 100%;">
                                                        <input type="text" maxlength="17" style="margin-left:10px;width:255px;" autocorrect="off" autocapitalize="off" class="input_macaddr_table" name="rule_mac" onClick="hideClients_Block();" onKeyPress="return validator.isHWAddr(this,event)" placeholder="ex: <% nvram_get("lan_hwaddr"); %>">
                                                        <img id="pull_arrow" height="14px;" src="/images/unfold_more.svg" onclick="pullLANIPList(this);" title="<#select_client#>">
                                                        <div id="ClientList_Block_PC" class="clientlist_dropdown"></div>
                                                    </div>
												</td>
												<td width="35%">
													<select class="input_option" name="rule_mode" id="client_modesel"></select>
												</td>
												<td width="15%"><input class="add_btn" type="button" onClick="addRow_main(64)" value=""></td>
											</tr>
										</table>
										<!-- Client list -->
										<div id="mainTable_Block"></div>
										<div class="apply_gen">
											<input name="button" type="button" class="button_gen" onclick="applyRule()" value="<#CTL_apply#>"/>
										</div>
									</td>
								</tr>
							</tbody>
						</table>
					</td>
				</tr>
			</table>
			<!--===================================Ending of Main Content===========================================-->
		</td>
		<td width="10" align="center" valign="top">&nbsp;</td>
	</tr>
</table>
<div id="footer"></div>
</form>
</body>
</html>
