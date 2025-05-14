# Gettext 多语言记录  

一般来说，使用`gettext` 为程序添加多语言支持时，需要遵循一些标准的步骤。  


## 最小可用示例  

### C++ 源码  
```cpp
#include <iostream>
#include <locale.h>
#include <libintl.h>

#define _(STRING) gettext(STRING)

int main() {
    setlocale(LC_ALL, "");  // 指定语言，例如en_US，zh_CN，zh_CN.UTF-8 等
    bindtextdomain("myapp", "./locale"); // .locale 是语言包的路径，可以是相对路径也可以是绝对路径
    textdomain("myapp");
    // 上面的myapp 可以改成自己喜欢的名字，但是文件路径需要遵循以下规则  
    // ./locale/{zh_CN}/LC_MESSAGES/{myapp}.mo  
    // 其中.mo 可以通过以下步骤生成

    std::cout << _("Hello, world!") << std::endl;
    return 0;
}
```  

### 提取、翻译   
```bash  
# 从main.cpp 中提取待翻译的内容
xgettext --from-code=UTF-8 -k_ main.cpp -o messages.pot

# 同时提取多个文件
find . -name "*.cpp" | xargs xgettext --from-code=UTF-8 -k_ -o messages.pot
```

通过[poedit](https://poedit.net/) 可以打开.pot 文件，并将翻译结果保存到相应的路径下。  

## 静态变量翻译
上面内容不太重要，多试试就好了。但是静态变量的翻译就很有意思了。  
```c++
#include <iostream>
#include <locale.h>
#include <libintl.h>

#define _(STRING) gettext(STRING)

const char * bye = _("Goodbye!");

int main() {
    setlocale(LC_ALL, "");  // 指定语言，例如en_US，zh_CN，zh_CN.UTF-8 等
    bindtextdomain("myapp", "./locale"); // .locale 是语言包的路径，可以是相对路径也可以是绝对路径
    textdomain("myapp");

    std::cout << _("Hello, world!") << std::endl;  // 可以正常显示对应语言
    std::cout << bye << std::endl;  // 无法正常显示对应语言
    return 0;
}
```

因为`bye` 是在`main()` 之前初始化的，所以在`main()` 执行时就获取不到对应的译文了。为此，可以封装一个函数：  
```cpp
#define _(STRING) _translate(STRING)
void _translate(const char* msg) {
    setlocale(LC_ALL, "");  // 指定语言，例如en_US，zh_CN，zh_CN.UTF-8 等
    bindtextdomain("myapp", "./locale"); // .locale 是语言包的路径，可以是相对路径也可以是绝对路径
    textdomain("myapp");

    return gettext(msg);
}
```  

### 多语言初始化标识符
每次运行`_()` 前都设置下语言就好了。但是这样会造成额外的开销，为此我们可以通过一个全局的标识符`gettext_initialized` 来表示语言是否初始化过了。
```cpp
#define _(STRING) _translate(STRING)

bool gettext_initialized = false;

void ensure_gettext_initialized() {
    if (!gettext_initialized) {
        setlocale(LC_ALL, "");
        bindtextdomain("gmsh", "./locale");
        textdomain("gmsh");
        gettext_initialized = true;
    }
}
const char* _translate(const char* msg) {
    ensure_gettext_initialized();
    return gettext(msg);
}
```  

### 多线程安全  
其实上面的代码在大部分情况下已经可用了，但是多线程的情况下可能会有一丢丢的问题（但仔细想想其实无关紧要，最多是多初始化几次而已）。为此，可以添加线程锁，以下是完整代码（未作验证）：  
```cpp
#include <iostream>
#include <locale.h>
#include <libintl.h>
#include <mutex>

#define _(STRING) _translate(STRING)
std::mutex gettext_mutex;
bool gettext_initialized = false;

void ensure_gettext_initialized() {
    std::lock_guard<std::mutex> lock(gettext_mutex);
    if (!gettext_initialized) {
        setlocale(LC_ALL, "");  // 也可用export LC_MESSAGES=zh_CN.UTF-8 指定语言
        bindtextdomain("myapp", "./locale");
        textdomain("myapp");
        gettext_initialized = true;
    }
}

const char* _translate(const char* msg) {
    ensure_gettext_initialized();
    return gettext(msg);
}


const char* messages[] = {
    _("Hello"),
    _("Goodbye!")
};
const char * bye = _("Goodbye!");

int main() {
    std::cout << _("Hello, world!") << std::endl;
    
    std::cout << bye << std::endl; 
    for (auto key : messages) {
        std::cout << key << std::endl;  // 运行时翻译
    }
    return 0;
}
```  

该操作甚至可以翻译数组`messages[]` 中的字符串。😮