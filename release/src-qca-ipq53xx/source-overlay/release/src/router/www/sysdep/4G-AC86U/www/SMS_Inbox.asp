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
<title><#Web_Title#> - SMS</title>
<link rel="stylesheet" type="text/css" href="index_style.css">
<link rel="stylesheet" type="text/css" href="form_style.css">
<link rel="stylesheet" type="text/css" href="other.css">
<link rel="stylesheet" type="text/css" href="sms.css">
<script type="text/javaScript" src="js/jquery.js"></script>
<script type="text/javascript" src="state.js"></script>
<script type="text/javascript" src="general.js"></script>
<script type="text/javascript" src="popup.js"></script>
<script type="text/javascript" src="help.js"></script>
<script type="text/javascript" src="./js/httpApi.js"></script>
<style>
.msg_container{
	display: flex;
	justify-content: flex-start;
	align-items: center;
	margin-left: 5px;
	width: 50%;
	flex-grow: 1;
	flex-wrap: wrap;
	cursor: pointer;
}
</style>
<script>

<% login_state_hook(); %>

var message_array = [];

var delete_array = [];
var save_message = 0;

function initial(){

	show_menu();
	get_all_messages();
}

function get_all_messages(){
	$.ajax({
		url: "/do_sms_read_msg.cgi",
		type: "POST",
		error: function(){
			console.log("get_all_message error");
		},
		success: function() {
			message_array = httpApi.hookGet("sms_get_message", true);
			show_messages();
		}
	});
}

function show_messages(){
	var i = 0;

	$("#message_records").empty();
	$("#message_records").css("height", "auto");

	if(message_array.length < 1){
		$("<div>")
			.attr({"id": "no_message_row"})
			.addClass("table_row no_message")
			.html("No message.")
			.appendTo($("#message_records"));
	}
	else{
		for(i = 0; i < message_array.length; i++){
			add_message_record(message_array[i])
		}
	}
}

function add_message_record(msg_record){
	var parentObj = $("#message_records");
	var recordObj =
			$("<div>")
			.attr({"id": "record" + msg_record.id})


	if(msg_record.user_status == 1){
		recordObj.addClass("table_row read");
	}
	else
		recordObj.addClass("table_row unread");

	recordObj.appendTo(parentObj);

	var selectContainerObj =
			$("<div>")
			.addClass("select_container")
			.appendTo(recordObj);

	var selectObj =
			$("<div>")
			.attr({"id": msg_record.id})
			.addClass("msg_select")
			.appendTo(selectContainerObj);

	selectObj.click(
		function(){
			if(selectObj.hasClass("msg_select_checked")){
				for(var i = 0; i < delete_array.length; i++){
					if(delete_array[i] == this.id){
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

	var timeContainerObj =
			$("<div>")
			.addClass("received_time")
			.appendTo(recordObj);

		$("<div>")
			.html(timeConverter(msg_record.time))
			.appendTo(timeContainerObj);


	var phoneNumberContainerObj =
			$("<div>")
			.addClass("phone_number")
			.appendTo(recordObj);

		$("<div>")
			.html(parseUnicodeToChar(msg_record.phone_number))
			.appendTo(phoneNumberContainerObj);

	var msgContainerObj =
			$("<div>")
			.addClass("msg_container")
			.appendTo(recordObj);

	var msgTextObj =
		$("<div>")
		.attr({"id": "msg" + msg_record.id})
		.addClass("msg_text text_ellipsis")
		.html(parseUnicodeToChar(msg_record.message))
		.appendTo(msgContainerObj);

	var msgMoreObj =
		$("<div>")
		.attr({"id": "msgMore" + msg_record.id})
		.addClass("arrow_container")
		.appendTo(msgContainerObj);

		$("<div>")
		.addClass("down_arrow")
		.appendTo(msgMoreObj);

	msgContainerObj.click(
		function(){
			msgMoreObj.toggleClass("down_arrow_up");
			msgTextObj.toggleClass("text_ellipsis");
			recordObj.removeClass("unread").addClass("read");
			for(var i = 0; i < message_array.length; i++){
				if(message_array[i].id == msg_record.id){
					message_array[i].user_status = 1;
					update_user_status(message_array[i].id, 1);
				}
			}
		});
}

function select_all(){
	if($("#select_all").hasClass("msg_select_checked") == false){
		$("#select_all").addClass("msg_select_checked");
		for(var i = 0; i < message_array.length; i++){
			var selectObj = $("#"+message_array[i].id);
			if(selectObj.hasClass("msg_select_checked") == false){
				selectObj.addClass("msg_select_checked");
				delete_array.push(message_array[i].id.toString());
			}
		}
	}
	else{
		$("#select_all").removeClass("msg_select_checked");
		for(var i = 0; i < message_array.length; i++){
			var selectObj = $("#"+message_array[i].id);
			if(selectObj.hasClass("msg_select_checked")){
				selectObj.removeClass("msg_select_checked");
				for(var j = 0; j < delete_array.length; j++){
					if(delete_array[j] == message_array[i].id.toString()){
						delete_array.splice(j, 1);
						break;
					}
				}
			}
		}
	}
}

function update_user_status(id, status){
	$.ajax({
		url: "/sms_update_userstatus.cgi",
		type: "POST",
		data: {
			"id": id,
			"user_status": status
		},
		error: function(){
			console.log("update_user_status error");
		},
		success: function() {

		}
	});
}

function do_delete_message(idx){
	$.ajax({
		url: "/do_sms_delete_msg.cgi",
		type: "POST",
		data: {
			"id": idx
		},
		error: function(){
			console.log("delete_messages error");
		},
		success: function() {

		}
	});
}

function delete_msg(){
	if($("#select_all").hasClass("msg_select_checked")){
		do_delete_message("all");
		message_array.length = 0;
	}
	else{
		for(var i = 0; i < delete_array.length; i++){
			for(var j = 0; j < message_array.length; j++){
				if( delete_array[i] == message_array[j].id){
					message_array.splice(j, 1);
					do_delete_message(delete_array[i]);
					break;
				}
			}
		}
	}

	delete_array.length = 0;
	if($("#select_all").hasClass("msg_select_checked"))
		$("#select_all").removeClass("msg_select_checked");

	show_messages();

}

function timeConverter(str){ //21/01/06,09:49:17+32 => 2021/01/06 0909:49:17
	var time_str = "";
	var year = '20'+ str.substring(0, 2);

	time_str = year + str.substring(2, str.indexOf('+')).replace(',', " ");
	return time_str;
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

<div id="LoadingBar" class="popup_bar_bg">
<table cellpadding="5" cellspacing="0" id="loadingBarBlock" class="loadingBarBlock" align="center">
	<tr>
		<td height="80">
		<div id="loading_block1" class="Bar_container">
			<span id="proceeding_img_text"></span>
			<div id="proceeding_img"></div>
		</div>
		<div id="loading_block2" style="margin:5px auto; width:85%;"></div>
		</td>
	</tr>
</table>
<!--[if lte IE 6.5]><iframe class="hackiframe"></iframe><![endif]-->
</div>

<div id="Loading" class="popup_bg"></div>

<iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>

<form method="post" name="form" id="ruleForm" action="/start_apply.htm" target="hidden_frame" autocomplete="off">
<input type="hidden" name="productid" value="<% nvram_get("productid"); %>">
<input type="hidden" name="current_page" value="SMS_Inbox.asp">
<input type="hidden" name="next_page" value="">
<input type="hidden" name="modified" value="0">
<input type="hidden" name="action_mode" value="apply">
<input type="hidden" name="action_script" value="">
<input type="hidden" name="action_wait" value="10">
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
		<table width="760px" border="0" cellpadding="5" cellspacing="0" class="FormTitle" id="FormTitle" style="-webkit-border-radius: 3px;-moz-border-radius: 3px;border-radius:3px;">
		<tbody>
		<tr>
			<td bgcolor="#4D595D" valign="top" height="680px">
				<div>&nbsp;</div>
				<div style="width:745px">
					<span class="formfonttitle">SMS - <#SMS_inbox#></span>
				</div>
				<div style="margin:5px;" class="splitLine"></div>
				<div style="width: 745px; height: auto; background-color: #32393B; margin-left: 5px; border-radius: 5px;">
					<div id="function_row" style="display: flex; align-items: center; height: 40px; margin-left: 5px;">
						<div class="msg_delete" onclick="delete_msg();"></div>
					</div>
					<div class="table_title">
						<div class="select_container">
							<div id="select_all" class="msg_select" onclick="select_all();"></div>
						</div>
						<div class="received_time">
							<div><#SMS_time#></div>
						</div>
						<div class="phone_number">
							<div><#Phone_Number#></div>
						</div>
						<div class="msg_container">
							<div><#SMS_msg#></div>
						</div>
					</div>
					<div id="message_records"></div>
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

<form method="post" name="simact_form" action="/apply.cgi" target="hidden_frame">
<input type="hidden" name="action_mode" value="">
<input type="hidden" name="action_script" value="">
<input type="hidden" name="action_wait" value="">
<input type="hidden" name="sim_pincode" value="">
<input type="hidden" name="sim_newpin" value="">
<input type="hidden" name="sim_puk" value="">
<input type="hidden" name="save_pin" value="">
<input type="hidden" name="g3err_pin" value="0">
<input type="hidden" name="wan_unit" value="">
<input type="hidden" name="sim_order" value="">
<input type="hidden" name="simdetect" value="">
</form>
</body>
</html>
