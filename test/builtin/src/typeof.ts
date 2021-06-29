globalThis.addon.typeofUndefined(undefined)
globalThis.addon.typeofNull(null)
globalThis.addon.typeofBoolean(true)
globalThis.addon.typeofBoolean(false)

globalThis.addon.typeofNumber(0)
globalThis.addon.typeofNumber(-0)
globalThis.addon.typeofNumber(1)
globalThis.addon.typeofNumber(-1)
globalThis.addon.typeofNumber(100)
globalThis.addon.typeofNumber(2121)
globalThis.addon.typeofNumber(-1233)
globalThis.addon.typeofNumber(986583)
globalThis.addon.typeofNumber(-976675)
globalThis.addon.typeofNumber(98765432213456789876546896323445679887645323232436587988766545658)
globalThis.addon.typeofNumber(-4350987086545760976737453646576078997096876957864353245245769809)
globalThis.addon.typeofNumber(Number.MIN_SAFE_INTEGER)
globalThis.addon.typeofNumber(Number.MAX_SAFE_INTEGER)
globalThis.addon.typeofNumber(Number.MAX_SAFE_INTEGER + 10)
globalThis.addon.typeofNumber(Number.MIN_VALUE)
globalThis.addon.typeofNumber(Number.MAX_VALUE)
globalThis.addon.typeofNumber(Number.MAX_VALUE + 10)
globalThis.addon.typeofNumber(Number.POSITIVE_INFINITY)
globalThis.addon.typeofNumber(Number.NEGATIVE_INFINITY)
globalThis.addon.typeofNumber(Number.NaN)

globalThis.addon.typeofString('')
globalThis.addon.typeofString('Hello, world!')
globalThis.addon.typeofString('测试')

globalThis.addon.typeofObject({})
globalThis.addon.typeofObject([])
globalThis.addon.typeofObject(new Number(100))
globalThis.addon.typeofObject(new String('String'))
// typeof /s/ === 'function'; // Chrome 1-12 , 不符合 ECMAScript 5.1
// typeof /s/ === 'object'; // Firefox 5+ , 符合 ECMAScript 5.1
globalThis.addon.typeofObject(/s/)
// Symbol 会导致 polyfill 被添加，typeof 实际上是 'object'
// globalThis.addon.typeofInvalidArg(Symbol('Symbol')

export { }