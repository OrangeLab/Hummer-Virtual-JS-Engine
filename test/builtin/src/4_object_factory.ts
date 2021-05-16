const obj1 = globalThis.addon('hello');
const obj2 = globalThis.addon('world');
globalThis.assert.strictEqual(`${obj1.msg} ${obj2.msg}`, 'hello world');

export { }