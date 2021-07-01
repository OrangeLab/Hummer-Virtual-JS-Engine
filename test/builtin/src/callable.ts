let flag = false
globalThis.addon.runWithCNullThis(function () {
    flag = true
    globalThis.assert(this === globalThis)
})
globalThis.assert(flag)

flag = false
globalThis.addon.run(function () {
    flag = true
    globalThis.assert(this === globalThis)
})
globalThis.assert(flag)

const hello = {}
globalThis.addon.runWithThis(function () {
    globalThis.assert(this === hello)
}, hello)

globalThis.addon.runWithArgument(function (...args: string[]) {
    globalThis.assert(args.length === 2)
    globalThis.assert(args[0] === 'hello')
    globalThis.assert(args[1] === 'world')
}, 'hello', 'world')

try {
    globalThis.addon.throwWithArgument(null)
} catch (e) {
    globalThis.assert(e === null)
}

try {
    globalThis.addon.throwWithArgument("Error")
} catch (e) {
    globalThis.assert(e === 'Error')
}

const errorObject = {}

try {
    globalThis.addon.throwWithArgument(errorObject)
} catch (e) {
    globalThis.assert(e === errorObject)
}

try {
    new globalThis.addon.ctor(null)
} catch (e) {
    globalThis.assert(e === null)
}

try {
    new globalThis.addon.ctor("Error")
} catch (e) {
    globalThis.assert(e === 'Error')
}

try {
    new globalThis.addon.ctor(errorObject)
} catch (e) {
    globalThis.assert(e === errorObject)
}

globalThis.assert(globalThis.addon.runWithCatch(function () {
    throw null;
}) === null)

globalThis.assert(globalThis.addon.runWithCatch(function () {
    throw 'null';
}) === 'null')

globalThis.assert(globalThis.addon.runWithCatch(function () {
    throw errorObject;
}) === errorObject)

class TestClass {
    constructor(arg: unknown) {
        throw arg
    }
}

globalThis.assert(globalThis.addon.newWithCatch(TestClass, null) === null)
globalThis.assert(globalThis.addon.newWithCatch(TestClass, 'null') === 'null')
globalThis.assert(globalThis.addon.newWithCatch(TestClass, errorObject) === errorObject)

globalThis.assert(new globalThis.addon.returnWithArgument(undefined) instanceof globalThis.addon.returnWithArgument)
globalThis.assert(new globalThis.addon.returnWithArgument(null) instanceof globalThis.addon.returnWithArgument)
globalThis.assert(new globalThis.addon.returnWithArgument(100) instanceof globalThis.addon.returnWithArgument)
globalThis.assert(new globalThis.addon.returnWithArgument(true) instanceof globalThis.addon.returnWithArgument)
globalThis.assert(new globalThis.addon.returnWithArgument(false) instanceof globalThis.addon.returnWithArgument)
globalThis.assert(new globalThis.addon.returnWithArgument('string') instanceof globalThis.addon.returnWithArgument)
globalThis.assert(!(new globalThis.addon.returnWithArgument({}) instanceof globalThis.addon.returnWithArgument))

globalThis.assert(new globalThis.addon.returnWithThis() instanceof globalThis.addon.returnWithThis)
globalThis.assert(new globalThis.addon.returnWithCNull() instanceof globalThis.addon.returnWithCNull)

throw null

export { }