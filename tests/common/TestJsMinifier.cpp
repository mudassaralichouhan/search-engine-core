#include "../../src/common/JsMinifier.h"
#include <iostream>

int main() {
    JsMinifier& minifier = JsMinifier::getInstance();
    minifier.setEnabled(true);
    std::string js = R"(
        // This is a comment

        var x = 1; /* multi-line
        comment */ var y = 2;   
        var dd = "ali 123" + 1
        function foo() {   return x + y;   } // end of line comment
    )";
    std::string result = minifier.process(js);
    std::cout << "Minified JS: [" << result << "]\n";
    return 0;
} 