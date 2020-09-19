#pragma once
namespace utility {
// T must be: no-throw default constructible and no-throw destructible
template <typename T>
struct Singleton {
private:
    struct ObjectCreator {
        // This constructor does nothing more than ensure that Instance()
        //  is called before main() begins, thus creating the static
        //  T object before multithreading race issues can come up.
        ObjectCreator() { Singleton<T>::Instance(); }
        inline void DoNothing() const { }
    };
    static ObjectCreator create_object;

    Singleton();

public:
    typedef T ObjectType;

    // If, at any point (in user code), Singleton<T>::Instance()
    //  is called, then the following function is instantiated.
    static ObjectType& Instance() {
        // This is the object that we return a reference to.
        // It is guaranteed to be created before main() begins because of
        //  the next line.
        static ObjectType obj;

        // The following line does nothing else than force the instantiation
        //  of Singleton<T>::create_object, whose constructor is
        //  called before main() begins.
        create_object.DoNothing();

        return obj;
    }
};
template <typename T>
typename Singleton<T>::ObjectCreator
Singleton<T>::create_object;

} // namespace utility

#endif // SEEMMO_UTILITY_SINGLETON_HPP_
