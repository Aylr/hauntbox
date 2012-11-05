function cupcake(column_names){			//cupcake acutally formats and sends the data to the Arduino
	var post_data = "";					//initialize the POST string
	var counter = 0;					//use to insert commas after each row
	var rows = $(".row").length;		//count how many rows, use for delimiters

	for (var i=0, tot=column_names.length; i < tot; i++) {	//loop through columns passed to cupcake
		$(column_names[i]).each(function(){	//loop through a single column
			counter ++;						//increment counter
			post_data += $(this).val();		//append value of input
			if(counter == rows){			//append appropriate element or row delimiter
				post_data += ";";			//row delimiter
				counter = 0;				//reset counter
			}else{
				post_data += ",";			//element delimiter
			};
		});//end each sensor loop
	};//loop through each column

	//console.log("rows: " + rows);
	console.log(post_data);				//log to firebug
	alert("fix status return elelment");
	

	$.post("/testget", post_data, function(data){ 	//the AJAX bit
	//	console.log("posted " + post_data);
		$("#status_program").hide().html(data).fadeIn();
	});

};//end cupcake()