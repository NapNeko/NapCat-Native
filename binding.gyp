{  
"targets": [  
	{
        "win_delay_load_hook": "true",
		"target_name": "loader",  
		"sources": [ "./src/NapCatNative.cpp"],
		"cflags!": [ "-fno-exceptions" ],
        "cflags_cc!": [ "-fno-exceptions" ],
	}
    ]  
}