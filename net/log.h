#include<cstdio>

//... 宏的不定参

//调试信息

 //FILE__和LINE__是预定义的宏，分别表示当前文件名和行号，##__VA_ARGS__表示可变参数列表

#define LOG_DEBUG(fmt,...){\
    printf("[DEBUG][%s:%d]" fmt "\n",__FILE__, __LINE__,##__VA_ARGS__);\     
}
//错误信息
#define LOG_ERROR(fmt,...){\
    printf("[ERROR][%s:%d]" fmt "\n",__FILE__, __LINE__,##__VA_ARGS__);\   
}   
//致命信息   abort();//终止程序
#define LOG_FATAL(fmt,...){\
    printf("[FATAL][%s:%d]" fmt "\n",__FILE__, __LINE__,##__VA_ARGS__);\   
    abort();\  
}
