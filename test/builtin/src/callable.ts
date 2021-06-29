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

globalThis.addon.runWithArgument((...args: string[]) => {
    globalThis.assert(args[0] === 'hello')
    globalThis.assert(args[1] === 'world')
}, 'hello', 'world')

export { }