
#include <iostream>
#include <memory>
#include <string>

struct Foo {
    Foo(std::string name, int id = 0) : name(name), id(id) {
        std::cout << "Foo()" << name << "_" << id << std::endl;
    }
    ~Foo() {
        std::cout << "~Foo() " + name << std::endl;
    }

    int id;
    std::string name;
};

int main() {
    std::unique_ptr<Foo> foo = std::make_unique<Foo>("wang", 1);
    std::unique_ptr<Foo> foo2(std::move(foo));
    std::cout << "foo: " << foo.get() << std::endl;
    std::cout << "foo2: " << foo2.get() << "foo2->name:" + foo2->name << std::endl;

    std::shared_ptr<Foo> foo3 = std::make_shared<Foo>("li", 2);
    std::shared_ptr<Foo> foo4 = foo3;
    std::cout << "foo3: " << foo3.get() << ", use_count: " << foo3.use_count() << std::endl;
    std::cout << "foo4: " << foo4.get() << ", use_count: " << foo4.use_count() << std::endl;
    foo3.reset();
    std::cout << "foo3: " << foo3.get() << ", use_count: " << foo3.use_count() << std::endl;
    std::cout << "foo4: " << foo4.get() << ", use_count: " << foo4.use_count() << "foo4->name:" + foo4->name << std::endl;

    std::weak_ptr<Foo> foo5 = foo4;
    std::cout << "foo5 expired: " << foo5.expired() << "foo5 name:" << foo5.lock()->name << std::endl;
    foo4.reset();
    std::cout << "foo5 expired: " << foo5.expired() << std::endl;

    return 0;
}