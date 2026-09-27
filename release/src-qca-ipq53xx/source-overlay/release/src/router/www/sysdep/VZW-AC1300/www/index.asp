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
<title><#Web_Title#> - <#Adaptive_History#></title>
<link rel="stylesheet" type="text/css" href="index_style.css"> 
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="usp_style.css">
<link rel="stylesheet" type="text/css" href="css/element.css">
<link rel="stylesheet" type="text/css" href="device-map/device-map.css">
<script type="text/javascript" src="/js/jquery.js"></script>
<script type="text/javascript" src="/js/httpApi.js"></script>
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/help.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/client_function.js"></script>
<script>
var wlClient = new Array();
function initial(){
	show_menu();
	updateClientList();
	updateClientListBackground();
	setInterval(function(){
		updateClientListBackground();
	}, 1000*60*3);
}

function updateClientList(){
	$.ajax({
		url: '/update_clients.asp',
		dataType: 'script', 
		error: function(xhr) {
			setTimeout("updateClientList();", 1000);
		},
		success: function(response){
			handleClientData();
			genClientList();
			setTimeout("updateClientList();", 3000);	
		}   
	});
}

function updateClientListBackground() {
	document.networkmapdRefresh.submit();
}

function handleClientData(){
	wlClient = [];
	if(wl_info.band5g2_support){
		var band_table = ["Wired", "2.4 G", "5 G-1", "5 G-2"];
	}
	else{
		var band_table = ["Wired", "2.4 G", "5 G"];
	}
	
	for(i = 0; i < clientList.length; i += 1){
		clientIndex = clientList[i];
		var clientName = (clientList[clientIndex].nickName == "") ? clientList[clientIndex].name : clientList[clientIndex].nickName;

		var tempArray = [clientName, clientList[clientIndex].ip, clientList[clientIndex].mac, clientList[clientIndex].rssi, clientList[clientIndex].curTx, clientList[clientIndex].curRx];

		tempArray.push(band_table[clientList[clientIndex].isWL]);
		tempArray.push(clientList[clientIndex].ipMethod);
		wlClient.push(tempArray);
	}

	generateClientTable();
}

function generateClientTable(){
	var code = '';
	code += '<tr>';
	code += '<th style="width:250px"><#ParentalCtrl_username#></th>';
	code += '<th style="width:200px"><#vpn_client_ip#></th>';
	code += '<th style="width:140px"><#ParentalCtrl_hwaddr#></th>';
	code += '<th style="width:50px"><#wan_interface#></th>';
	code += '<th style="width:50px">Tx Rate (Mbps)</th>';
	code += '<th style="width:50px">Rx Rate (Mbps)</th>';
	code += '</tr>';

	for(i=0; i<wlClient.length; i++){
		if(wlClient[i][1] == "offline"){
			continue;
		}

		code += '<tr>';
		code += '<td style="250px">' + wlClient[i][0] + '</td>';
		/* IP*/
		code += '<td style="width:200px">';
		code += wlClient[i][1];
		code += '<span style="float:right;margin-top:-3px;margin-right:5px;background-color: #000;color:#FFF;" class="ipMethodTag">'+ wlClient[i][7] +'</span>';

		code += '</td>';
		/* End IP */
		code += '<td style="width:140px">' + wlClient[i][2] + '</td>';
		/* Interface */
		code += '<td style="width:50px"><div style="height:36px;width:36px">';
		if(wlClient[i][6] == "Wired"){
			code += '<div class="radioIcon radio_wired"></div>';
		}
		else{
			var rssi = 0;
			rssi = client_convRSSI(wlClient[i][3]);
			code += '<div class="radioIcon radio_' + rssi + '"></div>';
			code += '<div class="band_block">';
			var bandClass = (navigator.userAgent.toUpperCase().match(/CHROME\/([\d.]+)/)) ? "band_txt_chrome" : "band_txt";
			code += '<span class="'+ bandClass +'" style="color:#000;background-color: transparent;">'+wlClient[i][6]+'</span>';
			code += '</div>';
		}

		code += '</div></td>';
		/* End Interface */
		/* Tx */
		code += '<td style="text-align:center;width:50px">';
		if(wlClient[i][4] == ""){
			code += '-';
		}
		else{
			code += wlClient[i][4];
		}

		code += '</td>';
		/* End Tx*/
		/* Rx */
		code += '<td style="text-align:center;width:50px;">';
		if(wlClient[i][5] == ""){
			code += '-';
		}
		else{
			code += wlClient[i][5];
		}
		code += '</td>';
		/* End Rx */
		code += '</tr>';
	}

	$("#client_table").html(code);
}
</script>
</head>
<body onload="initial();" onunload="unload_body();">
<div id="TopBanner"></div>
<div id="Loading" class="popup_bg"></div>
<div id="hiddenMask" class="popup_bg" style="z-index:999;">
	<table cellpadding="5" cellspacing="0" id="dr_sweet_advise" class="dr_sweet_advise" align="center"></table>
	<!--[if lte IE 6.5]><iframe class="hackiframe"></iframe><![endif]-->
</div>
<iframe name="hidden_frame" id="hidden_frame" width="0" height="0" frameborder="0"></iframe>
<form method="post" name="networkmapdRefresh" action="/apply.cgi" target="hidden_frame">
<input type="hidden" name="action_mode" value="update_client_list">
<input type="hidden" name="action_script" value="">
<input type="hidden" name="action_wait" value="1">
<input type="hidden" name="current_page" value="httpd_check.xml">
<input type="hidden" name="next_page" value="httpd_check.xml">
</form>
<form method="post" name="form" action="/start_apply.htm" target="hidden_frame">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<input type="hidden" name="current_page" value="/index.asp">
<input type="hidden" name="next_page" value="/index.asp">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="">
<input type="hidden" name="action_wait" value="3">
<input type="hidden" name="flag" value="">
<table class="content" align="center" cellpadding="0" cellspacing="0">
	<tr>
		<td width="17">&nbsp;</td>
		<td valign="top" width="202">
			<div id="mainMenu"></div>
			<div id="subMenu"></div>
		</td>	
		<td valign="top">
			<div id="tabMenu" class="submenuBlock"></div>		
			<table width="98%" border="0" align="left" cellpadding="0" cellspacing="0">
				<tr>
					<td align="left" valign="top">				
						<table width="760px" border="0" cellpadding="5" cellspacing="0" bordercolor="#6b8fa3"  class="FormTitle" id="FormTitle">		
							<tr>
								<td bgcolor="#4D595D" colspan="3" valign="top">
									<div>&nbsp;</div>
									<div class="formfonttitle">Client List</div>
									<div style="margin-left:5px;margin-top:10px;margin-bottom:10px"><img src="/images/New_ui/export/line_export.png"></div>
									<div class="formfontdesc"></div>
									<div style="margin:5px">
										<table id="client_table" style="margin-left:0px;" width="99%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
										</table>
									</div>
								</td>
							</tr>
						</table>
					</td>  
				</tr>
			</table>
			<!--===================================End of Main Content===========================================-->
		</td>		
	</tr>
</table>
<div id="footer"></div>
</body>
</html>
