#include <iostream>
#include <vector>
#include <string>

struct Person {
    std::string name;
    int age;

    Person(const std::string& n, int a) : name(n), age(a) {
        std::cout << "Constructed: " << name << std::endl;
    }

    Person(const Person& other) : name(other.name), age(other.age) {
        std::cout << "Copy Constructed: " << name << std::endl;
    }

    Person(Person&& other) noexcept : name(std::move(other.name)), age(other.age) {
        std::cout << "Move Constructed: " << name << std::endl;
    }

    ~Person() {
        std::cout << "Destructed: " << name << std::endl;
    }
};

int main() {
    std::vector<Person> people;
    people.reserve(3); // 预留空间，避免多次扩容

    std::cout << "Emplacing Alice" << std::endl;
    people.emplace_back("Alice", 30); // 直接在容器内构造

    std::cout << "Emplacing Bob" << std::endl;
    people.emplace_back("Bob", 25); // 直接在容器内构造

    std::cout << "Pushing back Charlie" << std::endl;
    Person charlie("Charlie", 28);
    people.push_back(charlie); // 通过拷贝构造函数插入

    std::cout << "Pushing back Dave (move)" << std::endl;
    people.push_back(Person("Dave", 35)); // 通过移动构造函数插入

    std::cout << "Final list of people:" << std::endl;
    for (const auto& person : people) {
        std::cout << person.name << ", Age: " << person.age << std::endl;
    }

    return 0;
}