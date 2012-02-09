// Scripts for theHB
// consider building the gui.htm form via a loop that changes row numbers as
// the index increases. This may save slightly on bandwith from the server

$("document").ready(function(){
	
	build_form(10);
	
	function build_form(num){
		if(num == undefined){			//set default if num isn't passed w/ function
			num = 6;
		}
		if(num < 11){					//minor sanity check with number of rows
			for(i=1;i<=num;i++){		//loop through to build rows
				//build actual row here
				$("form").append("<div class=\"last\" id=\"row" + i + "\">row" + i + "</div>");
			}
		}else{
			alert("Form is too big. Please keep it less than 10 rows.");
		}
	}//end build_form()
});//end doc.ready