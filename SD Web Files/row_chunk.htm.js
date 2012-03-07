var append_rows = "";
console.log

for(i=1;i<=6;i++){
	//console.log(i);
	append_rows += "<!--begin row" + i + " -->\
	<div id=\"row" + i + "\" class=\"row last\">\
		When input \
		<select id=\"sensor" + i + "\" class=\"sensor\" name=\"sensor" + i + "\">\
			<option value=\"0\">-</option>\
			<option value=\"1\">A</option>\
			<option value=\"2\">B</option>\
			<option value=\"3\">C</option>\
			<option value=\"4\">D</option>\
			<option value=\"5\">E</option>\
			<option value=\"6\">F</option>\
		</select>\
		is\
		<select id=\"sensor_state" + i + "\" class=\"sensor_state\" name=\"sensor_state" + i + "\">\
			<option value=\"1\">on</option>\
			<option value=\"0\">off</option>\
		</select>\
		&nbsp;&nbsp;:&nbsp;&nbsp;\
		Wait\
		<div class=\"wait\">\
			<a href=\"#\" class=\"up\">+</a>\
			<a href=\"#\" class=\"down\">-</a>\
		</div>\
		<input type=\"text\" id=\"wait_time" + i + "\" class=\"wait_time\" name=\"wait_time" + i + "\" maxlength=\"3\" value=\"0\" />\
		second(s) then turn output\
		<select id=\"output" + i + "\" class=\"output\" name=\"output" + i + "\">\
			<option value=\"0\">-</option>\
			<option value=\"1\">1</option>\
			<option value=\"2\">2</option>\
			<option value=\"3\">3</option>\
			<option value=\"4\">4</option>\
			<option value=\"5\">5</option>\
			<option value=\"6\">6</option>\
		</select>\
		<select id==\"output_state" + i + "\" class=\"output_state\" name=\"output_state" + i + "\">\
			<option value=\"1\">on</option>\
			<option value=\"0\">off</option>\
		</select>\
		<select class=\"output_duration\">\
			<option value=\"constantly\">constantly.</option>\
			<option value=\"for\">for...</option>\
		</select>\
		<div class=\"duration_container\">\
			<div class=\"duration\">\
				<a href=\"#\" class=\"up\">+</a>\
				<a href=\"#\" class=\"down\">-</a>\
			</div>\
			<input type=\"text\" id=\"output_duration" + i + "\" class=\"output_duration\" name=\"output_duration" + i + "\" maxlength=\"3\" value=\"0\" />\
			second(s).\
		</div>\
		<span class=\"row_status\"></span>\
	</div>\
	<!--end row" + i + " -->";
};