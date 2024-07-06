{  
"targets": [  
	{  
		"target_name": "loader",  
		"sources": [ "./src/NapCatNative.cpp"],
		"cflags!": [ "-fno-exceptions" ],
        "cflags_cc!": [ "-fno-exceptions" ],
	}
    ]  
}