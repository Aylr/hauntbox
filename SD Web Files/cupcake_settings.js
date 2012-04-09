function cupcake_settings(column_names){
	var post_data = "";					//initialize the POST string
	var counter = 0;					//use to insert commas after each row

	for (var i=0, tot=column_names.length; i < tot; i++) {
		//console.log($(column_names[i]));			//log the current object
	    //console.log($(column_names[i]).length);		//log the current object length
	
		$(column_names[i]).each(function(){	//loop through a single column
			counter ++;						//increment counter
			post_data += $(this).val();		//append value of input
			if(counter == $(column_names[i]).length){			//append appropriate element or row delimiter
				post_data += ";";			//row delimiter
				counter = 0;				//reset counter
			}else{
				post_data += ",";			//element delimiter
			};
		});//each element loop
	};//loop through each column

	console.log(post_data);				//log to firebug

	/*$.post("http://192.168.1.9/testget",post_data,function(){ 	//the AJAX bit
	//	console.log("posted " + post_data);
	});
	*/
};//end cupcake()