#include <iostream>
#include <functional>

int main() {
    auto add = [](int a, int b) {
        return a + b;
    };

    std::cout << add(1, 2) << std::endl;

    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::for_each(vec.begin(), vec.end(), [](int n) {
        std::cout << n * 2 << " ";
    });
}