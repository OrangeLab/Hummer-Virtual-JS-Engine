const sym = globalThis.addon.New('test');
globalThis.assert.strictEqual(sym.toString(), 'Symbol(test)');

const myObj: { foo?: string } = {};
const fooSym = globalThis.addon.New('foo');
const otherSym = globalThis.addon.New('bar');
myObj.foo = 'bar';
myObj[fooSym] = 'baz';
myObj[otherSym] = 'bing';
globalThis.assert.strictEqual(myObj.foo, 'bar');
globalThis.assert.strictEqual(myObj[fooSym], 'baz');
globalThis.assert.strictEqual(myObj[otherSym], 'bing');

export { }