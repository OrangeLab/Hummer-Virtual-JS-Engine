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

export { }