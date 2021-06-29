globalThis.assert(globalThis.addon.getUndefined() === undefined)
globalThis.assert(globalThis.addon.getNull() === null)
globalThis.assert(globalThis.addon.getGlobal() === globalThis)
globalThis.assert(typeof globalThis.addon.true === "boolean" && globalThis.addon.true)
globalThis.assert(typeof globalThis.addon.false === 'boolean' && !globalThis.addon.false)

function testNumber(num: number) {
    globalThis.assert(Object.is(num, globalThis.addon.testNumber(num)))
}

testNumber(0)
testNumber(-0)
testNumber(1)
testNumber(-1)
testNumber(100)
testNumber(2121)
testNumber(-1233)
testNumber(986583)
testNumber(-976675)

testNumber(
    98765432213456789876546896323445679887645323232436587988766545658)
testNumber(
    -4350987086545760976737453646576078997096876957864353245245769809)
testNumber(Number.MIN_SAFE_INTEGER)
testNumber(Number.MAX_SAFE_INTEGER)
testNumber(Number.MAX_SAFE_INTEGER + 10)

testNumber(Number.MIN_VALUE)
testNumber(Number.MAX_VALUE)
testNumber(Number.MAX_VALUE + 10)

testNumber(Number.POSITIVE_INFINITY)
testNumber(Number.NEGATIVE_INFINITY)
testNumber(Number.NaN)

globalThis.assert(globalThis.addon.nullCString === '')
globalThis.assert(globalThis.addon.emptyString === '')
globalThis.assert(globalThis.addon.asciiString === 'Hello, world!')
globalThis.assert(globalThis.addon.utf8String === '测试')

globalThis.assert(globalThis.addon.getUndefined.name === '')
globalThis.assert(globalThis.addon.getNull.name === '')
globalThis.assert(globalThis.addon.getGlobal.name === 'getGlobal')
globalThis.assert(globalThis.addon.testNumber.name === '测试数字')

export { }