$(document).ready(function(){

	var currentMethod = $("form#myform").attr('method');
	var newMethod = "post"; //default to post
	var oldMethod = "get";
	
	$("h1").html("Current Method: " + currentMethod)
	
	$("#changeProtocol").click(function(){
		if($("#myform").attr('method') == "post"){
			newMethod = "get";
			oldMethod = "post"
		}else{
			newMethod = "post"
			oldMethod = "get";
		}
		
		$("#myform").attr('method',newMethod);
		$("#changeProtocol").html("Change to " + oldMethod);
		
		$("h1").html("Current Method: " + newMethod);
		return false;

	});
	
/*	$("#btnSave").click(function(){
		var data = $("#addRunner :input").serializeArray();
		$.post($("addRunner").attr('action'),data,function(json){
			if(json.status == "fail"){
				alert(json.message);
			}
			if(json.status == "success"){
				alert(json.message);
				clearInputs();
			}
		}, "json");
	});
	
	function clearInputs(){
		$("#myform :input").each(function(){	//find each input in the form
			$(this).value("");					//set each to blank
		});
	};
*/	
	//$("#myform").submit(function(){			//if the user presses submit prevent
	//	return false;							//html action to let jQuery do it
	//});
});
