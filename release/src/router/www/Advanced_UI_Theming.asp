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
<title><#Web_Title#> - UI Theming</title>
<link rel="stylesheet" type="text/css" href="index_style.css"> 
<link rel="stylesheet" type="text/css" href="form_style.css">
<script type="text/javascript" src="/js/jquery.js"></script>
<script language="JavaScript" type="text/javascript" src="/state.js"></script>
<script language="JavaScript" type="text/javascript" src="/general.js"></script>
<script language="JavaScript" type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" language="JavaScript" src="/help.js"></script>
<script>
	const isThemesEnabled = isSupport("themes"); 
	const custom_theme_name = '<% nvram_get("theme_name"); %>';

	function applyRule(){
		showLoading();
		document.form.submit();
	}

	function initial(){
		show_menu(); 
		change_theme_preview_image(custom_theme_name);
	}

	function change_theme_preview_image(custom_theme_name){
		// document.getElementById("custom_theme").style.display = (enabled == 1) ? "":"none";
	}

</script>
</head>
<body onload="initial();" onunLoad="return unload_body();" class="bg">
	<div id="TopBanner"></div>
	<div id="Loading" class="popup_bg"></div>
	<iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>
	<form method="post" name="form" id="ruleForm" action="/start_apply.htm" target="hidden_frame">
		<input type="hidden" name="productid" value="<% nvram_get("productid"); %>">
		<input type="hidden" name="current_page" value="Advanced_UI_Theming.asp">
		<input type="hidden" name="next_page" value="Advanced_UI_Theming.asp">
		<input type="hidden" name="next_host" value="">
		<input type="hidden" name="group_id" value="">
		<input type="hidden" name="modified" value="1">
		<input type="hidden" name="flag" value="">
		<input type="hidden" name="action_mode" value="">
		<input type="hidden" name="action_wait" value="">
		<input type="hidden" name="action_script" value="saveNvram">
		<input type="hidden" name="first_time" value="">
		<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
		<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">

		<table class="content" align="center" cellpadding="0" cellspacing="0">
			<tr>
				<td width="17">&nbsp;</td>		
				<td valign="top" width="202">				
					<div  id="mainMenu"></div>	
					<div  id="subMenu"></div>		
				</td>				
				<td valign="top">
					<div id="tabMenu" class="submenuBlock"></div>
					<!--===================================Beginning of Main Content===========================================-->		
					<table width="98%" border="0" align="left" cellpadding="0" cellspacing="0">
						<tr>
							<td valign="top" >
								<table width="760px" border="0" cellpadding="4" cellspacing="0" class="FormTitle" id="FormTitle">
									<tbody>
										<tr>
											<td bgcolor="#4D595D" valign="top"  >
												<div>&nbsp;</div>
												<div class="formfonttitle"><#menu5_6#> - UI Theme</div>
												<div style="margin:10px 0 10px 5px;" class="splitLine"></div>
												<div class="formfontdesc"></div>
												<table width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3"  class="FormTable">
													<tr>
														<th>Select theme:</th>
														<td>
															<select name="theme_name" class="input_option" onchange="change_theme_preview_image(this.value);">
																<option value=""    <% nvram_match("theme_name", "", "selected"); %>>NONE</option>
																<option value="default" <% nvram_match("theme_name", "DEFAULT", "selected"); %>>DEFAULT</option>
																<option value="one"     <% nvram_match("theme_name", "ONE", "selected"); %>>ONE</option>
															</select>
														</td>
													</tr>
												</table>
												<div class="apply_gen">
													<input type="button" class="button_gen" onclick="applyRule()" value="<#CTL_apply#>"/>
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
		</form>
		<div id="footer">			
		</div>
</body>
</html>
