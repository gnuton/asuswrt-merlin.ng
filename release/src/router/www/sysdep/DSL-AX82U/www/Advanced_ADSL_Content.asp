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
<title><#Web_Title#> - <#menu_dsl_setting#></title>
<link rel="stylesheet" type="text/css" href="index_style.css">
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="other.css">
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/help.js"></script>
<script type="text/javascript" src="/js/jquery.js"></script>
<script>

function initial(){
	show_menu();

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
<input type="hidden" name="action_script" value="restart_dsl_setting">
<input type="hidden" name="action_wait" value="20">
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

		<table width="99%" border="1" align="center" cellpadding="4" cellspacing="0" class="FormTable">
			<thead>
			<tr>
				<td colspan="2">Common Settings</td>
			</tr>
			</thead>
			<tr>
				<th>
					<#dslsetting_disc1#>
				</th>
				<td>
					<select class="input_option" name="dslx_modulation">
						<option value="0" <% nvram_match("dslx_modulation", "0", "selected"); %>>T1.413</option>
						<option value="1" <% nvram_match("dslx_modulation", "1", "selected"); %>>G.lite</option>
						<option value="2" <% nvram_match("dslx_modulation", "2", "selected"); %>>G.Dmt</option>
						<option value="3" <% nvram_match("dslx_modulation", "3", "selected"); %>>ADSL2</option>
						<option value="4" <% nvram_match("dslx_modulation", "4", "selected"); %>>ADSL2+</option>
						<option value="6" <% nvram_match("dslx_modulation", "6", "selected"); %>>VDSL2</option>
						<option value="5" <% nvram_match("dslx_modulation", "5", "selected"); %>>Auto Sync-Up</option>
					</select>
				</td>
			</tr>
			<tr>
				<th>
					<#dslsetting_disc2#>
				</th>
				<td>
					<select class="input_option" name="dslx_annex">
						<option value="0" <% nvram_match("dslx_annex", "0", "selected"); %>>ANNEX A</option>
						<option value="2" <% nvram_match("dslx_annex", "2", "selected"); %>>ANNEX A/L</option>
						<option value="3" <% nvram_match("dslx_annex", "3", "selected"); %>>ANNEX A/M</option>
						<option value="4" <% nvram_match("dslx_annex", "4", "selected"); %>>ANNEX A/L/M</option>
					</select>
				</td>
			</tr>
			<tr>
				<th>
					<a class="hintstyle" href="javascript:void(0);" onClick="openHint(25,2);"><#dslsetting_SRA#></a>
				</th>
				<td>
					<select class="input_option" name="dslx_sra">
						<option value="1" <% nvram_match("dslx_sra", "1", "selected"); %>><#btn_Enabled#></option>
						<option value="0" <% nvram_match("dslx_sra", "0", "selected"); %>><#btn_Disabled#></option>
					</select>
				</td>
			</tr>

			<tr>
				<th>
					<a class="hintstyle" href="javascript:void(0);" onClick="openHint(25,3);">Bitswap</a>
				</th>
				<td>
					<select class="input_option" name="dslx_bitswap">
						<option value="1" <% nvram_match("dslx_bitswap", "1", "selected"); %>><#btn_Enabled#></option>
						<option value="0" <% nvram_match("dslx_bitswap", "0", "selected"); %>><#btn_Disabled#></option>
					</select>
				</td>
			</tr>
			<tr>
				<th>
					<a class="hintstyle" href="javascript:void(0);" onClick="openHint(25,12);">G.INP (G.998.4)</a>
				</th>
				<td>
					<select class="input_option" name="dslx_ginp">
						<option value="1" <% nvram_match("dslx_ginp", "1", "selected"); %>><#btn_Enabled#></option>
						<option value="0" <% nvram_match("dslx_ginp", "0", "selected"); %>><#btn_Disabled#></option>
					</select>
				</td>
			</tr>
			<tr>
				<th>
					<#dslsetting_Stability_Adj#>
				</th>
				<td>
					<select id="dslx_snrm_offset" class="input_option" name="dslx_snrm_offset">
						<option value="0" <% nvram_match("dslx_snrm_offset", "0", "selected"); %>><#btn_Disabled#></option>
						<option value="16" <% nvram_match("dslx_snrm_offset", "16", "selected"); %>>1 dB</option>
						<option value="32" <% nvram_match("dslx_snrm_offset", "32", "selected"); %>>2 dB</option>
						<option value="48" <% nvram_match("dslx_snrm_offset", "48", "selected"); %>>3 dB</option>
						<option value="64" <% nvram_match("dslx_snrm_offset", "64", "selected"); %>>4 dB</option>
						<option value="80" <% nvram_match("dslx_snrm_offset", "80", "selected"); %>>5 dB</option>
						<option value="96" <% nvram_match("dslx_snrm_offset", "96", "selected"); %>>6 dB</option>
						<option value="112" <% nvram_match("dslx_snrm_offset", "112", "selected"); %>>7 dB</option>
						<option value="128" <% nvram_match("dslx_snrm_offset", "128", "selected"); %>>8 dB</option>
						<option value="144" <% nvram_match("dslx_snrm_offset", "144", "selected"); %>>9 dB</option>
						<option value="160" <% nvram_match("dslx_snrm_offset", "160", "selected"); %>>10 dB</option>
						<option value="176" <% nvram_match("dslx_snrm_offset", "176", "selected"); %>>11 dB</option>
						<option value="192" <% nvram_match("dslx_snrm_offset", "192", "selected"); %>>12 dB</option>
						<option value="208" <% nvram_match("dslx_snrm_offset", "208", "selected"); %>>13 dB</option>
						<option value="224" <% nvram_match("dslx_snrm_offset", "224", "selected"); %>>14 dB</option>
						<option value="240" <% nvram_match("dslx_snrm_offset", "240", "selected"); %>>15 dB</option>
						<option value="256" <% nvram_match("dslx_snrm_offset", "256", "selected"); %>>16 dB</option>
						<option value="272" <% nvram_match("dslx_snrm_offset", "272", "selected"); %>>17 dB</option>
						<option value="288" <% nvram_match("dslx_snrm_offset", "288", "selected"); %>>18 dB</option>
						<option value="304" <% nvram_match("dslx_snrm_offset", "304", "selected"); %>>19 dB</option>
						<option value="320" <% nvram_match("dslx_snrm_offset", "320", "selected"); %>>20 dB</option>
					</select>
				</td>
			</tr>
		</table>

		<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable" style="margin-top:10px;">
			<thead>
			<tr>
				<td colspan="2">VDSL Settings</td>
			</tr>
			</thead>
			<tr>
				<th>
					VDSL Profile
				</th>
				<td>
					<select class="input_option" name="dslx_vdsl_profile">
						<option value="0" <% nvram_match("dslx_vdsl_profile", "0", "selected"); %>>Multi mode</option>
						<option value="1" <% nvram_match("dslx_vdsl_profile", "1", "selected"); %>>8a</option>
						<option value="2" <% nvram_match("dslx_vdsl_profile", "2", "selected"); %>>8b</option>
						<option value="3" <% nvram_match("dslx_vdsl_profile", "3", "selected"); %>>8c</option>
						<option value="4" <% nvram_match("dslx_vdsl_profile", "4", "selected"); %>>8d</option>
						<option value="5" <% nvram_match("dslx_vdsl_profile", "5", "selected"); %>>12a</option>
						<option value="6" <% nvram_match("dslx_vdsl_profile", "6", "selected"); %>>12b</option>
						<option value="7" <% nvram_match("dslx_vdsl_profile", "7", "selected"); %>>17a</option>
						<option value="8" <% nvram_match("dslx_vdsl_profile", "8", "selected"); %>>30a</option>
						<option value="9" <% nvram_match("dslx_vdsl_profile", "9", "selected"); %>>35b</option>
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
	</tr>
</table>
			<!--===================================End of Main Content===========================================-->
	</td>

	<td width="10" align="center" valign="top">&nbsp;</td>
  </tr>
</table>
</form>

<div id="footer"></div>
</body>
</html>
