globalThis.assert(globalThis.addon.isArray([]))
globalThis.assert(globalThis.addon.getThis() === globalThis.addon)
class TestA {
    arg1?: number
    arg2?: string
    constructor(arg1: number, arg2: string) {
        this.arg1 = arg1
        this.arg2 = arg2
    }
}
class TestB {

}
const testB = globalThis.addon.new(TestB)
globalThis.assert(Object.getPrototypeOf(testB) === TestB.prototype)
const testA = globalThis.addon.new(TestA, 1, 'hello')
globalThis.assert(Object.getPrototypeOf(testA) === TestA.prototype)
globalThis.assert(testA.arg1 === 1);
globalThis.assert(testA.arg2 === 'hello')
{
    const object = {
        hello: 'world'
    }
    object[0] = 'hello'

    globalThis.assert(globalThis.addon.get(object, 'hello') === 'world')
    globalThis.assert(globalThis.addon.get(object, 0) === 'hello')

    globalThis.assert(globalThis.addon.has(object, 'hello'))
    globalThis.assert(globalThis.addon.has(object, 0))

    globalThis.assert(globalThis.addon.delete(object, 'hello'))
    globalThis.assert(!globalThis.addon.has(object, 'hello'))
}

{
    // 原型链
    const MyObject = function () {
        this.foo = 42
        this.bar = 43
        this[0] = 1
        this[1] = 2
    }
    MyObject.prototype.bar = 44
    MyObject.prototype[1] = 3
    MyObject.prototype.baz = 45
    MyObject.prototype[2] = 4;

    const obj = new MyObject()

    globalThis.assert(globalThis.addon.get(obj, 'foo') === 42)
    globalThis.assert(globalThis.addon.get(obj, 'bar') === 43)
    globalThis.assert(globalThis.addon.get(obj, 'baz') === 45)
    globalThis.assert(globalThis.addon.get(obj, 'toString') === Object.prototype.toString)
    globalThis.assert(globalThis.addon.has(obj, 'toString'))

    globalThis.assert(globalThis.addon.get(obj, 0) === 1)
    globalThis.assert(globalThis.addon.get(obj, 1) === 2)
    globalThis.assert(globalThis.addon.get(obj, 2) === 4)

    globalThis.assert(globalThis.addon.delete(obj, 'baz'))
    globalThis.assert(globalThis.addon.has(obj, 'baz'))
}

{
    // napi_set_property
    const testPropertyDescriptor = function (propertyDescriptor?: PropertyDescriptor) {
        globalThis.assert(!!propertyDescriptor)
        globalThis.assert(propertyDescriptor?.configurable)
        globalThis.assert(propertyDescriptor?.enumerable)
        globalThis.assert(propertyDescriptor?.writable)
        globalThis.assert(propertyDescriptor?.value)
        globalThis.assert(!propertyDescriptor?.get)
        globalThis.assert(!propertyDescriptor?.set)
    }
    const hello = {}
    globalThis.addon.set(hello, 'world', true)
    globalThis.addon.set(hello, 0, true)
    testPropertyDescriptor(Object.getOwnPropertyDescriptor(hello, 'world'))
    testPropertyDescriptor(Object.getOwnPropertyDescriptor(hello, 0))
}

export { }