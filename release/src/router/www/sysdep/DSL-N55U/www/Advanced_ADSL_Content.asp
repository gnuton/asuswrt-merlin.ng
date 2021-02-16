﻿<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<html xmlns:v>
<head>
<meta http-equiv="X-UA-Compatible" content="IE=Edge"/>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<link rel="shortcut icon" href="images/favicon.png">
<link rel="icon" href="images/favicon.png">
<title><#Web_Title#> - <#menu_dsl_setting#></title>
<link rel="stylesheet" type="text/css" href="index_style.css">
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="other.css">
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/help.js"></script>

<script>

function initial(){
	var dsl_modulation_x = "<% nvram_get("dslx_modulation"); %>";
	var dsl_annex_x = "<% nvram_get("dslx_annex"); %>";
	var dsl_model = 'annex_a';
	
	if (productid == "DSL-N55U-B") dsl_model = 'annex_b';

	if (dsl_modulation_x == 0)
		document.form.dslx_modulation[0].checked = true;
	else if (dsl_modulation_x == 1)
		document.form.dslx_modulation[1].checked = true;
	else if (dsl_modulation_x == 2)
		document.form.dslx_modulation[2].checked = true;
	else if (dsl_modulation_x == 3)
		document.form.dslx_modulation[3].checked = true;
	else if (dsl_modulation_x == 4)
		document.form.dslx_modulation[4].checked = true;
	else
		document.form.dslx_modulation[5].checked = true;

	if (dsl_model == 'annex_a') {
		if (dsl_annex_x == 0)
			document.form.dslx_annex[0].checked = true;
		else if (dsl_annex_x == 1)
			document.form.dslx_annex[1].checked = true;
		else if (dsl_annex_x == 2)
			document.form.dslx_annex[2].checked = true;
		else if (dsl_annex_x == 3)
			document.form.dslx_annex[3].checked = true;
		else if (dsl_annex_x == 4)
			document.form.dslx_annex[4].checked = true;
	}
	else {
		if (dsl_annex_x == 5)
			document.form.dslx_annex[0].checked = true;
		else if (dsl_annex_x == 6)
			document.form.dslx_annex[1].checked = true;
	}
	show_menu();
	hideXDSLSetting(document.form.dslx_testlab.value);
}

function applyRule(){
		if(valid_form()){
				showLoading();
				document.form.submit();
		}
}

function valid_form(){
	return true;
}

function disableOption(obj){
	obj.style.backgroundColor = "#CCCCCC";
	obj.disabled = true;
}

function enableOption(obj){
	obj.style.backgroundColor = "#576D73";
	obj.disabled = false;
}

function hideXDSLSetting(_value){
	if( _value == "AU" || _value == "BR" || _value == "GB" ){
		disableOption(document.form.dslx_snrm_offset);
	}
	else
	{
		enableOption(document.form.dslx_snrm_offset);
	}
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
<input type="hidden" name="current_page" value="Advanced_ADSL_Content.asp">
<input type="hidden" name="next_page" value="Advanced_ADSL_Content.asp">
<input type="hidden" name="group_id" value="">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="reboot">
<input type="hidden" name="action_wait" value="<% get_default_reboot_time(); %>">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">

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
		  <div class="formfonttitle"><#menu5_6#> - <#menu_dsl_setting#></div>
      <div style="margin: 10px 0 10px 5px;" class="splitLine"></div>
      <div class="formfontdesc"><#dslsetting_disc0#></div>

		<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
		<thead>
		<tr>
		<td colspan="7"><#dslsetting_disc1#></td>
		</tr>
		</thead>
		<tr>
		<td><input name="dslx_modulation" type="radio" value="0"> T1.413 </td>
		</tr>
		<tr>
		<td><input name="dslx_modulation" type="radio" value="1"> G.lite </td>
		</tr>
		<tr>
		<td><input name="dslx_modulation" type="radio" value="2"> G.DMT </td>
		</tr>
		<tr>
		<td><input name="dslx_modulation" type="radio" value="3"> ADSL2 </td>
		</tr>
		<tr>
		<td><input name="dslx_modulation" type="radio" value="4"> ADSL2+ </td>
		</tr>
		<tr>
		<td><input name="dslx_modulation" type="radio" value="5"> Auto Sync-Up </td>
		</tr>
		</table>
		<br>
		<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
		<thead>
		<tr>
		<td colspan="7"><#dslsetting_disc2#></td>
		</tr>
		</thead>
		<tr>
		<td><input name="dslx_annex" type="radio" value="0"> ANNEX A </td>
		</tr>
		<tr>
		<td><input name="dslx_annex" type="radio" value="1"> ANNEX I </td>
		</tr>
		<tr>
		<td><input name="dslx_annex" type="radio" value="2"> ANNEX A/L </td>
		</tr>
		<tr>
		<td><input name="dslx_annex" type="radio" value="3"> ANNEX M </td>
		</tr>
		<tr>
		<td><input name="dslx_annex" type="radio" value="4"> ANNEX A/I/J/L/M </td>
		</tr>
</table>
		<br>
<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
		<thead>
		<tr>
		<td colspan="7"><a class="hintstyle" href="javascript:void(0);" onClick="openHint(25,9);">Country-Specific Setting</a></td>
		</tr>
		</thead>
		<tr>
		<td>
			<select class="input_option" name="dslx_testlab" onchange="hideXDSLSetting(this.value);">
				<option value="disable" <% nvram_match("dslx_testlab", "disable", "selected"); %>><#btn_Disabled#></option>
				<option value="GB" <% nvram_match("dslx_testlab", "GB", "selected"); %>>United Kingdom</option>
				<option value="AU" <% nvram_match("dslx_testlab", "AU", "selected"); %>>Australia</option>
			</select>
		</td>
		</tr>
</table>
		<br>
<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
		<thead>
		<tr>
		<td colspan="7"><a class="hintstyle" href="javascript:void(0);" onClick="openHint(25,1);"><#dslsetting_Stability_Adj#></a></td>
		</tr>
		</thead>
		<tr>
		<td>
			<select class="input_option" name="dslx_snrm_offset">
				<option value="0" <% nvram_match("dslx_snrm_offset", "0", "selected"); %>><#btn_Disabled#></option>
				<option value="5120" <% nvram_match("dslx_snrm_offset", "5120", "selected"); %>>10 dB</option>
				<option value="4608" <% nvram_match("dslx_snrm_offset", "4608", "selected"); %>>9 dB</option>
				<option value="4096" <% nvram_match("dslx_snrm_offset", "4096", "selected"); %>>8 dB</option>
				<option value="3584" <% nvram_match("dslx_snrm_offset", "3584", "selected"); %>>7 dB</option>
				<option value="3072" <% nvram_match("dslx_snrm_offset", "3072", "selected"); %>>6 dB</option>
				<option value="2560" <% nvram_match("dslx_snrm_offset", "2560", "selected"); %>>5 dB</option>
				<option value="2048" <% nvram_match("dslx_snrm_offset", "2048", "selected"); %>>4 dB</option>
				<option value="1536" <% nvram_match("dslx_snrm_offset", "1536", "selected"); %>>3 dB</option>
				<option value="1024" <% nvram_match("dslx_snrm_offset", "1024", "selected"); %>>2 dB</option>
				<option value="512" <% nvram_match("dslx_snrm_offset", "512", "selected"); %>>1 dB</option>
				<option value="-512" <% nvram_match("dslx_snrm_offset", "-512", "selected"); %>>-1 dB</option>
				<option value="-1024" <% nvram_match("dslx_snrm_offset", "-1024", "selected"); %>>-2 dB</option>
				<option value="-1536" <% nvram_match("dslx_snrm_offset", "-1536", "selected"); %>>-3 dB</option>
				<option value="-2048" <% nvram_match("dslx_snrm_offset", "-2048", "selected"); %>>-4 dB</option>
				<option value="-2560" <% nvram_match("dslx_snrm_offset", "-2560", "selected"); %>>-5 dB</option>
				<option value="-3072" <% nvram_match("dslx_snrm_offset", "-3072", "selected"); %>>-6 dB</option>
				<option value="-3584" <% nvram_match("dslx_snrm_offset", "-3584", "selected"); %>>-7 dB</option>
				<option value="-4096" <% nvram_match("dslx_snrm_offset", "-4096", "selected"); %>>-8 dB</option>
				<option value="-4608" <% nvram_match("dslx_snrm_offset", "-4608", "selected"); %>>-9 dB</option>
				<option value="-5120" <% nvram_match("dslx_snrm_offset", "-5120", "selected"); %>>-10 dB</option>
			</select>
		</td>
		</tr>
</table> 
		<br>
<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
		<thead>
		<tr>
		<td colspan="7"><a class="hintstyle" href="javascript:void(0);" onClick="openHint(25,2);"><#dslsetting_SRA#></a></td>
		</tr>
		</thead>
		<tr>
		<td>
			<select class="input_option" name="dslx_sra">
				<option value="1" <% nvram_match("dslx_sra", "1", "selected"); %>><#btn_Enabled#></option>
				<option value="0" <% nvram_match("dslx_sra", "0", "selected"); %>><#btn_Disabled#></option>
			</select>
		</td>
		</tr>
</table>
		<br>
<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
		<thead>
		<tr>
		<td colspan="7"><a class="hintstyle" href="javascript:void(0);" onClick="openHint(25,3);">Bitswap</a></td>
		</tr>
		</thead>
		<tr>
		<td>
			<select class="input_option" name="dslx_bitswap">
				<option value="1" <% nvram_match("dslx_bitswap", "1", "selected"); %>><#btn_Enabled#></option>
				<option value="0" <% nvram_match("dslx_bitswap", "0", "selected"); %>><#btn_Disabled#></option>
			</select>
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
