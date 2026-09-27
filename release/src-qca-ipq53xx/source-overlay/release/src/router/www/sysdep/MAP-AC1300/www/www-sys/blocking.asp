<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="X-UA-Compatible" content="IE=Edge"/>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<link rel="shortcut icon" href="images/favicon.png">
<link rel="icon" href="images/favicon.png">
<title><#Web_Title#> - Blocking Page</title>
<script type="text/JavaScript" src="/js/jquery.js"></script>
<style>
body{
	color:#FFF;
	font-family: Arial;
}
.wrapper{
	background-size: 1330px 580px;
}
.title_name {
    font-family: Arial;
    font-weight: bold;
    font-size: 29px;
    color: #181818;
    text-align: center;
}
.title_text{
	width:520px;
	padding-top: 8px;
	margin:auto;
}
.er_title{
	font-family: Arial;
    font-weight: bold;
    font-size: 18px;
    color: #181818;
}
.prod_madelName{
    padding-top: 12px;
    text-align: center;
    font-family: Arial;
    font-weight: bold;
    font-size: 15px;
    color: #181818;
}
.div_img {
	text-align:center;
	padding-top: 20px;
}

.blocking_img{
	width:372px;
	height:303px;
	background-position:center;
	background-attachment:fixed;
	background:url(images/New_ui/asus_hive_block_internet_web.png) no-repeat center;
	margin:auto;
}
.p1{
	font-family: Arial;
    font-weight: bold;
    font-size: 17px;
    color: #181818;
	width: 480px;
}
ul {
	margin-left: -21px;
	color: #28d1a5;
}

li span {
	font-family: Arial;
	font-weight:bold;
	font-size: 15px;
	color: #181818;
}
.button{
	background-color:#279FD9;
	/*background:rgba(255,255,255,0.1);
	border: solid 1px #6e8385;*/
	border-radius: 4px ;
	transition: visibility 0s linear 0.218s,opacity 0.218s,background-color 0.218s;
	height: 68px;
	width: 300px;
	font-size: 28px;
	color:#fff;
	color:#000\9; /* IE6 IE7 IE8 */
	text-align: center;
	float:right;
	margin:25px 0 0 78px;
	line-height:68px;
}
.form_input{
	background-color:rgba(255,255,255,0.2);
	border-radius: 4px;
	padding:26px 22px;
	width: 480px;
	border: 0;
	height:25px;
	color:#fff;
	color:#000\9; /* IE6 IE7 IE8 */
	font-size:28px
}
.nologin{
	margin:10px 0px 0px 78px;
	background-color:rgba(255,255,255,0.2);
	padding:10px 0px;
	line-height:36px;
	border-radius: 5px;
	width: 600px;
	border: 0;
	color:#fff;
	color:#000\9; /* IE6 IE7 IE8 */
	font-size: 18px;
}
.div_table{
	display:table;
}
.div_tr{
	display:table-row;
}
.div_td{
	display:table-cell;
}
.title_gap{
	margin:10px 0px 0px 78px;
}
.img_gap{
	padding-right:30px;
	vertical-align:middle;
}
.password_gap{
	margin:30px 0px 0px 78px;
}
.error_hint{
	color: rgb(255, 204, 0);
	margin:10px 0px -10px 78px;
	font-size: 18px;
	font-weight: bolder;
}
.main_field_gap{
	margin:80px auto 0;
}
ul{
	margin: 0;
}
#wanLink{
	cursor: pointer;
}
.button_background{
	background-color: transparent;
}
.tm_logo{
	background:url('images/New_ui/tm_logo_1.png') no-repeat;
	width:487px;
	height:90px;
	background-size: 80%;
	margin-left: -20px;
}
.desc_info{
	font-weight:bold;
}
#tm_block{
	margin: 20px 20px;
	font-size: 15px;
	color: #181818;
}
.block_info_div{
	width: 60%;
	margin: 0px 200px auto auto;
}
/*for mobile device*/
@media screen and (max-width: 1000px){
	.title_name {
		font-size: 24px;
		width: 90%;
	}
	.er_title{
	font-size: 16px;
	}
	.prod_madelName{
		padding-top:9px;
		width: 100%;
	}
	.div_img {
		padding-top: 3px;
		width: 100%;
	}

	.blocking_img{
		background-size: 75%;
	}
	.p1{
		font-size: 16px;
		width:100%;
	}
	li span {
		font-size: 14px;
	}
	.title_text{
		width: 100%;
	}
	.form_input{
		padding:13px 11px;
		width: 100%;
		height:25px;
		font-size:16px
	}
	.button{
		height: 50px;
		width: 100%;
		font-size: 16px;
		text-align: center;
		float:right;
		margin: 25px -30px 0 0;
		line-height:50px;
		padding: 0 10px;
	}
	.nologin{
		margin-left:10px;
		padding:5px;
		line-height:18px;
		width: 90%;
		font-size:14px;
	}
	.error_hint{
		margin-left:10px;
	}
	.main_field_gap{
		width:80%;
		margin:10px auto 0;
		/*margin:30px auto 0;*/
	}
	.title_gap{
		margin-left:15px;
	}
	.password_gap{
		margin-left:15px;
	}
	.img_gap{
		padding-right:0;
		vertical-align:middle;
	}
	ul{
		margin-left:-20px;
	}
	li{
		margin: 10px 0;
	}
	.tm_logo{
		width: 100%;
		background-repeat: no-repeat;
		background-size: 100%;
	}
	.block_info_div{
		width: 88%;
		margin: auto 0;
	}
}
</style>
<script type="text/javascript">
/* String replace &#39; with ' for dict */
function stringSafeGet(str){
	return str.replace(new RegExp("&#39;", 'g'), "'");
}
var bwdpi_support = ('<% nvram_get("rc_support"); %>'.search('bwdpi') == -1) ? false : true;
var mac_parameter = '<% get_parameter("mac"); %>'.toUpperCase();
var casenum = '<% get_parameter("cat_id"); %>';
var flag = '<% get_parameter("flag"); %>';
var block_info = '<% bwdpi_redirect_info(); %>';
if(block_info != "")
	block_info = JSON.parse(block_info);
var client_list_array = '<% get_client_detail_info(); %>';
var custom_name = decodeURIComponent('<% nvram_char_to_ascii("", "custom_clientlist"); %>').replace(/&#62/g, ">").replace(/&#60/g, "<");
var category_info = [
			["Parental Controls", "1", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult1#>", "<#block_cate_PC1#>"],
                        ["Parental Controls", "2", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult1#>", "<#block_cate_PC2#>"],
                        ["Parental Controls", "3", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult1#>", "<#block_cate_PC3#>"],
                        ["Parental Controls", "4", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult1#>", "<#block_cate_PC4#>"],
                        ["Parental Controls", "5", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult1#>", "<#block_cate_PC5#>"],
                        ["Parental Controls", "6", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult1#>", "<#block_cate_PC6#>"],
                        ["Parental Controls", "8", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult1#>", "<#block_cate_PC8#>"],                         
                         ["Parental Controls", "9", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult2#>", "<#block_cate_PC9#>"],
                         ["Parental Controls", "10", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult2#>", "<#block_cate_PC10#>"],
                         ["Parental Controls", "14", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult2#>", "<#block_cate_PC14#>"],
                         ["Parental Controls", "15", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult2#>", "<#block_cate_PC15#>"],
                         ["Parental Controls", "16", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult2#>", "<#block_cate_PC16#>"],
                         ["Parental Controls", "25", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult2#>", "<#block_cate_PC25#>"],
                         ["Parental Controls", "26", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult2#>", "<#block_cate_PC26#>"],
                        ["Parental Controls", "11", "<#AiProtection_filter_Adult#>", "<#AiProtection_filter_Adult3#>", "<#block_cate_PC11#>"],
                         ["Parental Controls", "24", "<#AiProtection_filter_message#>", "<#AiProtection_filter_Adult4#>", "<#block_cate_PC24#>"],
                        ["Parental Controls", "51", "<#AiProtection_filter_message#>", "<#AiProtection_filter_Adult5#>", "<#block_cate_PC51#>"],
                         ["Parental Controls", "53", "<#AiProtection_filter_message#>", "<#AiProtection_filter_Adult6#>", "<#block_cate_PC53#>"],
                         ["Parental Controls", "89", "<#AiProtection_filter_message#>", "<#AiProtection_filter_Adult6#>", "<#block_cate_PC89#>"],
                        ["Parental Controls", "42", "<#AiProtection_filter_message#>", "<#AiProtection_filter_Adult7#>", "<#block_cate_PC42#>"],
                         ["Parental Controls", "56", "<#AiProtection_filter_p2p#>", "<#AiProtection_filter_p2p1#>", "<#block_cate_PC56#>"],
                         ["Parental Controls", "70", "<#AiProtection_filter_p2p#>", "<#AiProtection_filter_p2p1#>", "<#block_cate_PC70#>"],
                         ["Parental Controls", "71", "<#AiProtection_filter_p2p#>", "<#AiProtection_filter_p2p1#>", "<#block_cate_PC71#>"],         
                        ["Parental Controls", "57", "<#AiProtection_filter_p2p#>", "<#AiProtection_filter_p2p2#>", "<#block_cate_PC57#>"],
                         ["Parental Controls", "69", "<#AiProtection_filter_stream#>", "<#AiProtection_filter_stream2#>", "<#block_cate_PC69#>"],
                        ["Parental Controls", "23", "<#AiProtection_filter_stream#>", "<#AiProtection_filter_stream3#>", "<#block_cate_PC23#>"],

                         ["Home Protection", "91", "Anti-Trojan detecting and blocked", "", "<#block_cate_HP91#>"],
                        ["Home Protection", "39", "Malicious site blocked", "", "<#block_cate_HP39#>"],
                        ["Home Protection", "73", "Malicious site blocked", "", "<#block_cate_HP73#>"],
                        ["Home Protection", "74", "Malicious site blocked", "", "<#block_cate_HP74#>"],
                        ["Home Protection", "75", "Malicious site blocked", "", "<#block_cate_HP75#>"],
                        ["Home Protection", "76", "Malicious site blocked", "", "<#block_cate_HP76#>"],
                        ["Home Protection", "77", "Malicious site blocked", "", "<#block_cate_HP77#>"],
                        ["Home Protection", "78", "Malicious site blocked", "", "<#block_cate_HP78#>"],
                        ["Home Protection", "79", "Malicious site blocked", "", "<#block_cate_HP79#>"],
                        ["Home Protection", "80", "Malicious site blocked", "", "<#block_cate_HP80#>"],
                        ["Home Protection", "81", "Malicious site blocked", "", "<#block_cate_HP81#>"],
                        ["Home Protection", "82", "Malicious site blocked", "", "<#block_cate_HP82#>"],
                        ["Home Protection", "83", "Malicious site blocked", "", "<#block_cate_HP83#>"],
                        ["Home Protection", "84", "Malicious site blocked", "", "<#block_cate_HP84#>"],
                        ["Home Protection", "85", "Malicious site blocked", "", "<#block_cate_HP85#>"],
                        ["Home Protection", "86", "Malicious site blocked", "", "<#block_cate_HP86#>"],
                        ["Home Protection", "88", "Malicious site blocked", "", "<#block_cate_HP88#>"],
                        ["Home Protection", "92", "Malicious site blocked", "", "<#block_cate_HP92#>"],
                        ["Home Protection", "94", "Malicious site blocked", "", "<#block_cate_HP94#>"],
                        ["Home Protection", "95", "Malicious site blocked", "", "<#block_cate_HP95#>"]
		];

var target_info = {
	name: "",
	mac: "",
	url: "",
	category_id: "",
	category_type: "",
	content_category: "",
	desc: ""
}

function initial(){
	get_target_info();
	show_information();
}

function get_target_info(){
	var client_list_row = client_list_array.split('<');
	var custom_name_row = custom_name.split('<');
	var match_flag = 0;

	for(i=1;i<custom_name_row.length;i++){
		var custom_name_col = custom_name_row[i].split('>');
		if(custom_name_col[1] == block_info[0] || custom_name_col[1] == mac_parameter){
			target_info.name = custom_name_col[0];
			match_flag =1;
		}
	}

	if(match_flag == 0){
		for(i=1;i< client_list_row.length;i++){
			var client_list_col = client_list_row[i].split('>');
			if(client_list_col[3] == block_info[0] || client_list_col[3] == mac_parameter){
				target_info.name = client_list_col[1];
			}
		}
	}

	if(casenum != ""){		//for AiProtection
		target_info.mac = block_info[0];
		target_info.url = block_info[1];
		target_info.category_id = block_info[2];
		get_category_info();
	}
	else{		//for Parental Controls (Time Scheduling)
		target_info.mac = mac_parameter;
		target_info.desc = "<#block_TS_desc#>";
	}
}

function get_category_info(){
	var cat_id = target_info.category_id;
	var category_string = "";
	for(i=0;i< category_info.length;i++){
		if(category_info[i][1] == cat_id){
			category_string = category_info[i][2];
			if(category_info[i][3] != ""){
				category_string += " - " + category_info[i][3];
			}

			target_info.category_type = category_info[i][0];
			target_info.content_category = category_string;
			target_info.desc = category_info[i][4];
		}
	}
}

function show_information(){
	var code = "";
	var code_suggestion = "";
	var code_title = "";
	var parental_string = "";

	code = "<ul>";
	code += "<li><div><span class='desc_info'>" + target_info.desc + "</span></div></li>";
	code += "<li><div><span class='desc_info'>Device Name (MAC): ";
	if(target_info.name == target_info.mac)
		code += target_info.name;
	else
		code += target_info.name + " (" + target_info.mac.toUpperCase() + ")";

	code += "</span></div></li>";
	if(casenum != "")
		code += "<li><div><span class='desc_info'>URL: " + target_info.url +"</span></div></li>";

	if(target_info.category_type == "Parental Controls")
		code += "<li><div><span class='desc_info'><#AiProtection_filter_category#> :</span>" + target_info.content_category + "</div></li>";

	code += "</ul>";
	document.getElementById('detail_info').innerHTML = code;

	if(target_info.category_type == "Parental Controls"){	//Webs Apps filter
		code_title = "<div class='er_title' style='height:auto;'><#block_PC_Title#></div>";
		code_suggestion = "<ul>";
		code_suggestion += stringSafeGet("<li><span><#block_PC_suggest1#></span></li>");
		code_suggestion += "<li><span><#block_TS_suggest3#></span></li>";
		code_suggestion += '<li><#AiProtection_parental_control_report_desc#><a href="https://global.sitesafety.trendmicro.com/index.php" target="_blank"><#AiProtection_parental_control_report_tm#></a></li>';
		code_suggestion += "</ul>";
		document.getElementById('tm_block').style.display = "none";
	}
	else if(target_info.category_type == "Home Protection"){
		code_title = "<div class='er_title' style='height:auto;'><#block_HP_Title#></div>";
		code_suggestion = "<ul>";
		code_suggestion += "<li><span><#block_HP_suggest1#></span></li>";
		code_suggestion += '<li><#AiProtection_sites_report_desc#><a href="https://global.sitesafety.trendmicro.com/index.php" target="_blank"><#AiProtection_sites_report_tm#></a></li>';
		code_suggestion += "</ul>";
		document.getElementById('tm_block').style.display = "";
	}
	else if(flag != ""){
		code_title = "<div class='er_title' style='height:auto;'><#web_redirect_message#></div>";
		document.getElementById('main_reason').innerHTML = "<#web_redirect_fail_reason0#>";
		code = "";
		code += "<div><#web_redirect_reason_limited#></div>";

		document.getElementById('detail_info').innerHTML = code;

		code_suggestion = "<ul>";
		code_suggestion += "<li><span><#web_redirect_reason_limited_1#></span></li>";
		code_suggestion += "<li><span><#web_redirect_reason_limited_2#></span></li>";
		code_suggestion += "</ul>";
	}
	else{		//for Parental Control(Time Scheduling)
		code_title = "<div class='er_title' style='height:auto;'><#block_TS_title#></div>"
		code_suggestion = "<ul>";
		if(bwdpi_support)
			parental_string = "<#Time_Scheduling#>";
		else
			parental_string = stringSafeGet("<#Parental_Control#>");

		code_suggestion += "<li><span><#block_TS_suggest1#> "+ parental_string +" <#block_TS_suggest2#></span></li>";
		code_suggestion += "<li><span><#block_TS_suggest3#></span></li>";
		code_suggestion += "</ul>";
		document.getElementById('tm_block').style.display = "none";
	}

	document.getElementById('page_title').innerHTML = code_title;
	document.getElementById('suggestion').innerHTML = code_suggestion;
}
/*
//wait app
function change_request_button(){
	var block_wait_list_row = block_wait_list.split('<');

	for(i=1;i<block_wait_list_row.length;i++){
		var block_wait_name_col = block_wait_list_row[i].split('>');
		if(block_wait_name_col[0] == mac_parameter && (block_wait_name_col[1] - Math.floor(Date.now()/1000) > 0))
			block_wait_match_flag = 1;
	}

	if(block_wait_match_flag == 1){
		disable_button(1);
		remaining_time = block_wait_name_col[1] - Math.floor(Date.now()/1000);
		rtime_obj=document.getElementById("rtime");
		rtime_obj.innerHTML=secondsToTime(remaining_time);
		countdownid = window.setInterval(countdownfunc,1000);
	}
}

function disable_button(val){
	if(val == 1){
		document.getElementById("rtime").style.display ="";
		document.getElementById("request_button_span").className = "wait_circle_span";
		document.getElementById("request_button").className = "wait_button";
		document.getElementById('request_button_span').innerHTML = "REQUESTING";
	}else{
		document.getElementById("rtime").style.display ="none";
		document.getElementById("request_button_span").className = "circle_span";
		document.getElementById("request_button").className = "button";
		document.getElementById('request_button_span').innerHTML = "REQUEST";
	}
}

function countdownfunc(){
	rtime_obj.innerHTML=secondsToTime(remaining_time);
	if (remaining_time==0){
		clearInterval(countdownid);
		setTimeout(refresh_page(), 2000);
	}
	remaining_time--;
}

function refresh_page(){
	top.location.href='/blocking.asp?mac='+mac_parameter;
}

function secondsToTime(secs)
{
    secs = Math.round(secs);
    var hours = Math.floor(secs / (60 * 60));

    var divisor_for_minutes = secs % (60 * 60);
    var minutes = Math.floor(divisor_for_minutes / 60);

    var divisor_for_seconds = divisor_for_minutes % 60;
    var seconds = Math.ceil(divisor_for_seconds);

    var time_str = (minutes<10?("0"+minutes):minutes)+":"+(seconds<10?("0"+seconds):seconds);
    return time_str;
}

function get_target_info(){

	var block_hostname = "";
	var client_list_row = client_list_array.split('<');
	var custom_name_row = custom_name.split('<');
	var match_flag = 0;

	for(i=1;i<custom_name_row.length;i++){
		var custom_name_col = custom_name_row[i].split('>');
		if(custom_name_col[1] == block_info[0] || custom_name_col[1] == mac_parameter){
			target_info.name = custom_name_col[0];
			match_flag =1;
		}
	}

	if(match_flag == 0){
		for(i=1;i< client_list_row.length;i++){
			var client_list_col = client_list_row[i].split('>');
			if(client_list_col[3] == block_info[0] || client_list_col[3] == mac_parameter){
				target_info.name = client_list_col[1];
			}
		}
	}

	target_info.mac = mac_parameter;

	if(target_info.name == target_info.mac)
		block_hostname += target_info.name;
	else
		block_hostname += target_info.name + " (" + target_info.mac.toUpperCase() + ")";

	document.getElementById('block_hostname').innerHTML = block_hostname;
}

function unblock_request(){
	if(block_wait_match_flag == 1) return;
	var timestap_now = Date.now();
	var timestr_start = Math.floor(timestap_now/1000);
	var timestr_end = Math.floor(timestap_now/1000) + 3600;
	var timestap_start = new Date(timestap_now);
	var timestap_end = new Date(timestap_now + 3600000);
	var timestap_start_hours = (timestap_start.getHours()<10? "0" : "") + timestap_start.getHours();
	var timestap_end_hours = (timestap_end.getHours()<10? "0" : "") + timestap_end.getHours();
	var timestap_start_Min = (timestap_start.getMinutes()<10? "0" : "") + timestap_start.getMinutes();
	var timestap_end_Min = (timestap_end.getMinutes()<10? "0" : "") + timestap_end.getMinutes();

	var interval = timestap_start.getDay().toString() + timestap_end.getDay().toString() + timestap_start_hours + timestap_end_hours + timestap_start_Min + timestap_end_Min;

	document.form.CName.value = target_info.name;
	document.form.mac.value = target_info.mac;
	document.form.timestap.value = timestr_end;
	document.form.interval.value = interval;
	document.form.submit();
}
*/

</script>
</head>
<body class="wrapper" onload="initial();">
	<div class="div_table main_field_gap">
		<div class="title_name">Oops!</div>
		<div class="title_name">
			<div id="page_title" class="title_text"></div>
		</div>
		<div class="prod_madelName"><#Web_Title2#></div>
		<div class="div_img">
			<div class="blocking_img"></div>
		</div>
		<div class="block_info_div">
		<div id="main_reason" class="p1 title_gap"><#block_DetailInfo#></div>
		<div>
			<div class="p1 title_gap"></div>
			<div class="nologin">
				<div id="detail_info"></div>
			</div>
		</div>

		<div class="p1 title_gap"><#web_redirect_suggestion0#> :</div>
		<div>
			<div class="p1 title_gap"></div>
			<div class="nologin">
				<div id="case_content"></div>
				<div id="suggestion"></div>
				<div id="tm_block" style="display:none">
					<div><#block_HP_suggest2#></div>
					<div class="tm_logo"></div>
				</div>
			</div>
		</div>
		</div>
		<div id="go_btn" class='button' style="display:none;"><#btn_go#></div>
	</div>
</body>
</html>
