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
<title><#Web_Title#> - <#SMS_send_msg#></title>
<link rel="stylesheet" type="text/css" href="index_style.css">
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="other.css">
<link rel="stylesheet" type="text/css" href="sms.css">
<script type="text/javaScript" src="js/jquery.js"></script>
<script type="text/javascript" src="state.js"></script>
<script type="text/javascript" src="general.js"></script>
<script type="text/javascript" src="popup.js"></script>
<script type="text/javascript" src="help.js"></script>
<script type="text/javascript" src="validator.js"></script>
<script type="text/javascript" src="./js/httpApi.js"></script>
<style>
.draft_container{
	display: flex;
	justify-content: flex-start;
	align-items: center;
	margin-left: 5px;
	width: 50%;
	flex-grow: 1;
	flex-wrap: wrap;
}

.edit_icon{
	width: 24px;
	height: 24px;
	background: url(images/gameprofile/edit.svg) no-repeat center;
	background-size: 65%;
	cursor: pointer;
}

.new_div{
	display: flex;
	cursor: pointer;
	align-items: center;
	justify-content: flex-end;
	margin-right: 10px;
}

.new_div:hover .new_sms{
	width: 30px;
	height: 30px;
	background: url(images/edit_active.svg) no-repeat center;
	background-size: 100%;
}
</style>
<script>

<% login_state_hook(); %>
var draft_array = httpApi.hookGet("sms_get_draft", true);
var delete_array = [];
var DRAFT_MAXNUM = 5;

function initial(){

	show_menu();
	$("#max_draft_num").text(DRAFT_MAXNUM);
	show_drafts();

}

function show_drafts(){
	var i = 0;

	$("#draft_records").empty();
	$("#draft_records").css("height", "auto");
	if(draft_array.length < 1){
		$("<div>")
			.attr({"id": "no_message_row"})
			.addClass("table_row no_message")
			.html("<#SMS_no_draft#>")
			.appendTo($("#draft_records"));
	}
	else{
		for(i = 0; i < draft_array.length; i++){
			add_draft_record(draft_array[i]);
		}
	}
}

function add_draft_record(draft_record){
	var parentObj = $("#draft_records");
	var recordObj =
			$("<div>")
			.attr({"id": "record" + draft_record.id})

	recordObj.addClass("table_row");
	recordObj.appendTo(parentObj);

	var selectContainerObj =
			$("<div>")
			.addClass("select_container")
			.appendTo(recordObj);

	var selectObj =
			$("<div>")
			.attr({"id": draft_record.id})
			.addClass("msg_select")
			.appendTo(selectContainerObj);

	selectObj.click(
		function(){
			if(selectObj.hasClass("msg_select_checked")){
				for(var i = 0; i < delete_array.length; i++){
					if(delete_array[i].id == this.id){
						delete_array.splice(i, 1);
					}
				}
				if($("#select_all").hasClass("msg_select_checked"))
					$("#select_all").removeClass("msg_select_checked");
			}
			else{
				delete_array.push(this.id);
			}
			selectObj.toggleClass("msg_select_checked");
		});

	var phoneNumberContainerObj =
			$("<div>")
			.addClass("phone_number")
			.appendTo(recordObj);

		$("<div>")
			.html(parseUnicodeToChar(draft_record.phone_number))
			.appendTo(phoneNumberContainerObj);

	var msgContainerObj =
			$("<div>")
			.addClass("draft_container")
			.appendTo(recordObj);

	var msgTextObj =
		$("<div>")
		.attr({"id": "msg" + draft_record.id})
		.addClass("msg_text")
		.html(parseUnicodeToChar(draft_record.message))
		.appendTo(msgContainerObj);

	var editObj =
		$("<div>")
		.attr({"id": "edit_" + draft_record.id})
		.addClass("arrow_container")
		.appendTo(msgContainerObj);

		$("<div>")
		.addClass("edit_icon")
		.appendTo(editObj);

	editObj.click(
		function(){
			edit_sms(1, this.id);
		});
}

function edit_sms(_show, id){
	if(_show){
		$("#btn_div").css("display", "block");
		$("#loadingIcon").css("display", "none");
		if(id == undefined){
			$("#phone_number").val("");
			$("#sms_text").val("");
			$("#save_btn").removeAttr('onclick');
			$("#save_btn").unbind('click');
			$("#save_btn").click(function(){
					do_save_draft();
				}
			);

			$("#send_btn").removeAttr('onclick');
			$("#send_btn").unbind('click');
			$("#send_btn").click(function(){
					do_send_message();
				}
			);
		}
		else{
			var draft_id = id.split('_')[1];
			for(var i = 0; i < draft_array.length; i++){
				if(draft_array[i].id == draft_id){
					$("#phone_number").val(parseUnicodeToChar(draft_array[i].phone_number));
					$("#sms_text").val(parseUnicodeToChar(draft_array[i].message));
					break;
				}
			}
			$("#save_btn").removeAttr('onclick');
			$("#save_btn").unbind('click');
			$("#save_btn").click(function(){
					do_save_draft(draft_id);
				}
			);

			$("#send_btn").removeAttr('onclick');
			$("#send_btn").unbind('click');
			$("#send_btn").click(function(){
					do_send_message(draft_id);
				}
			);


		}
		$("#sms_edit_div").fadeIn(500);
	}
	else{
		$("#sms_edit_div").fadeOut(500);
	}

}

function select_all(){
	if($("#select_all").hasClass("msg_select_checked") == false){
		$("#select_all").addClass("msg_select_checked");
		for(var i = 0; i < draft_array.length; i++){
			var selectObj = $("#"+draft_array[i].id);
			if(selectObj.hasClass("msg_select_checked") == false){
				selectObj.addClass("msg_select_checked");
				delete_array.push(draft_array[i].id.toString());
			}
		}
	}
	else{
		$("#select_all").removeClass("msg_select_checked");
		for(var i = 0; i < draft_array.length; i++){
			var selectObj = $("#"+draft_array[i].id);
			if(selectObj.hasClass("msg_select_checked")){
				selectObj.removeClass("msg_select_checked");
				for(var j = 0; j < delete_array.length; j++){
					if(delete_array[j] == draft_array[i].id.toString()){
						delete_array.splice(j, 1);
						break;
					}
				}
			}
		}
	}
}

function delete_draft(){
	for(var i = 0; i < delete_array.length; i++){
		for(var j = 0; j < draft_array.length; j++){
			if( delete_array[i] == draft_array[j].id){
				draft_array.splice(j, 1);
				break;
			}
		}
	}
	delete_array.length = 0;

	if($("#select_all").hasClass("msg_select_checked")){
		$("#select_all").removeClass("msg_select_checked");
	}
	update_draft();
}

function do_delete_draft(idx){
	$.ajax({
		url: "/do_sms_delete_msg.cgi",
		type: "POST",
		data: {
			"id": idx
		},
		error: function(){
			console.log("do_delete_draft error");
		},
		success: function() {

		}
	});
}

function do_send_message(idx){
	if(check_mobile_number(document.form.phone_number)){
		$("#btn_div").css("display", "none");
		$("#loadingIcon").css("display", "block");
		setTimeout(function(){
				var phone_number = parseStrToUnicode(document.form.phone_number.value);
				var msg = parseStrToUnicode(document.form.sms_text.value);

				$.ajax({
					url: "/do_sms_send_msg.cgi",
					type: "POST",
					data: {
						"phone_number": phone_number,
						"msg":msg
					},
					error: function(){
						console.log("send message error");
					},
					success: function() {
						var send_result = httpApi.nvramGet(["ret_sms_send"], true).ret_sms_send;
						if(send_result == "1"){
							setTimeout(function(){alert("<#SMS_succeed_send#>");}, 500);
							edit_sms(0);
							for(var i = 0; i < draft_array.length; i++){
								if(draft_array[i].id == idx){
									draft_array.splice(i, 1);
									update_draft();
									break;
								}
							}
							show_drafts();
						}
						else{
							alert("<#SMS_fail_send#>");
							$("#btn_div").css("display", "block");
							$("#loadingIcon").css("display", "none");
						}
					}
				});
			},100);
	}
}

function update_draft(){
	var tmp_str = "";
	var draft_array_str = "";
	for(var i = 0; i < draft_array.length; i++){
		tmp_str = JSON.stringify(draft_array[i]);
		if(i == 0)
			draft_array_str = "[";

		draft_array_str += tmp_str;

		if(draft_array.length > 1 && i != draft_array.length - 1)
			draft_array_str += ",";

		if(i == draft_array.length - 1)
			draft_array_str += "]";
	}

	$.ajax({
		url: "/do_sms_save_draft.cgi",
		type: "POST",
		data: {
			"sms_draft": draft_array_str
		},
		error: function(){
			console.log("send message error");
		},
		success: function() {
			if($("#sms_edit_div").css("display") == "block"){
				edit_sms(0);
			}
			show_drafts();
		}
	});
}

function do_save_draft(idx){
	var phone_number = "";
	var msg = "";

	if(draft_array.length >= DRAFT_MAXNUM){
			alert("<#SMS_Max_draft_hint#>");
			edit_sms(0);
			return;
	}
	if(document.form.sms_text.value == "")
		alert("<#SMS_blank_hint#>");
	else if(check_mobile_number(document.form.phone_number)){
		phone_number = parseStrToUnicode(document.form.phone_number.value);
		msg = parseStrToUnicode(document.form.sms_text.value);
		$("#btn_div").css("display", "none");
		$("#loadingIcon").css("display", "block");
		if(idx != undefined){
			var draftObj = {
				"id": idx,
				"phone_number": phone_number,
				"message": msg
			};
			for(var i = 0; i < draft_array.length; i++){
				if(draft_array[i].id == idx){
					draft_array.splice(i, 1, draftObj);
					break;
				}
			}
		}
		else{
			var dateTime = Date.now();
			var timestamp = Math.floor(dateTime / 1000);
			var draftObj = {
				"id": dateTime,
				"phone_number": phone_number,
				"message": msg
			};
			draft_array.push(draftObj);
		}

		update_draft();
	}
}

function check_mobile_number(obj){
	//if(!validator.isMobileNumber(obj.value)){
	if(obj.value == ""){
		alert("<#SMS_wrong_number#>");
		obj.focus();
		obj.select();
		return false;
	}

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

<form method="post" name="form" action="" target="hidden_frame" autocomplete="off">
<input type="hidden" name="productid" value="<% nvram_get("productid"); %>">
<input type="hidden" name="current_page" value="SMS_Send.asp">
<input type="hidden" name="next_page" value="">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get("preferred_lang"); %>">
<input type="hidden" name="firmver" value="<% nvram_get("firmver"); %>">
<div id="sms_edit_div" class="popup_div">
	<div style="display: flex; justify-content: flex-end;">
	<div class="close_icon" onclick="edit_sms(0);"></div>
	</div>
	<table  width="100%" border="1" align="center" cellpadding="4" cellspacing="0" bordercolor="#6b8fa3" class="FormTable">
			<tr>
				<th><#Phone_Number#></th>
				<td align="left">
					<input type="text" name="phone_number" id="phone_number" maxlength="16" class="input_15_table" style="margin-left: -5px;" autocorrect="off" autocapitalize="off">
				</td>
			</tr>
			<tr>
				<th>
					<div style="width: 100%; display: flex; justify-content: flex-start; align-items: center; flex-wrap: wrap">
						<div style="margin-right: 5px;"><#SMS_msg#></div>
						<div>(<#List_limit#>&nbsp;70)</div>
					</div>
				</th>
				<td>
					<textarea rows="8" name="sms_text" id="sms_text" class="textarea_ssh_table" style="margin-left: -5px; resize: none;" cols="65" maxlength="70"></textarea>
				</td>
			</tr>
	</table>
	<div style="text-align: center; margin-top: 10px; height: 35px; display: flex; justify-content: center; align-items: center;">
		<div id="btn_div">
			<input type="button" id="send_btn" class="button_gen" value="<#btn_send#>">
			<input type="button" id="save_btn" class="button_gen" value="<#CTL_onlysave#>">
		</div>
		<div id="loadingIcon" style="width: 24px; height: 24px; background: url(/images/InternetScan.gif) no-repeat center;
	background-size: 100%; display: none;"></div>
	</div>
</div>
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
			<table width="760px" border="0" cellpadding="5" cellspacing="0" class="FormTitle" id="FormTitle" style="-webkit-border-radius: 3px;-moz-border-radius: 3px;border-radius:3px;">
		<tbody>
		<tr>
			<td bgcolor="#4D595D" valign="top" height="680px">
				<div>&nbsp;</div>
				<div style="width:730px">
					<table width="730px">
						<tr>
							<td align="left">
								<span class="formfonttitle">SMS - <#SMS_send_msg#></span>
							</td>
						</tr>
					</table>
				</div>
				<div style="margin:5px;" class="splitLine"></div>
				<div style="width: 100%; height: 30px; margin-top: 10px; display: flex; justify-content: space-between;">
					<div style="width: 50%; display: flex; justify-content: flex-start; align-items: flex-end; font-family: Verdana, MS UI Gothic, MS P Gothic, sans-serif; font-size: 12px; font-weight: bolder;">
						<div style="margin-left: 10px;"><#SMS_draft#></div>
						<div style="margin-left: 5px;">(<#List_limit#>&nbsp;<span id="max_draft_num"></span>)</div>
					</div>
					<div class="new_div" onclick="edit_sms(1); " title="<#SMS_new_msg#>"><div class="new_sms"></div><div style="font-weight: bolder; font-family: Verdana, MS UI Gothic, MS P Gothic, sans-serif;"><#SMS_new#></div></div>
				</div>
				<div style="width: 745px; height: auto; background-color: #32393B; margin-left: 5px; border-radius: 5px;">
					<div id="function_row" style="display: flex; align-items: center; height: 40px; margin-left: 5px;">
						<div class="msg_delete" onclick="delete_draft();"></div>
					</div>
					<div class="table_title">
						<div class="select_container">
							<div id="select_all" class="msg_select" onclick="select_all();"></div>
						</div>
						<div class="phone_number">
							<div><#Phone_Number#></div>
						</div>
						<div class="msg_container">
							<div><#SMS_msg#></div>
						</div>
					</div>
					<div id="draft_records"></div>
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
