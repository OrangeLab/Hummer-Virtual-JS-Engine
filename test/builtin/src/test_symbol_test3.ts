globalThis.assert.notStrictEqual(globalThis.addon.New(), globalThis.addon.New());
globalThis.assert.notStrictEqual(globalThis.addon.New('foo'), globalThis.addon.New('foo'));
globalThis.assert.notStrictEqual(globalThis.addon.New('foo'), globalThis.addon.New('bar'));

const foo1 = globalThis.addon.New('foo');
const foo2 = globalThis.addon.New('foo');
const object = {
  [foo1]: 1,
  [foo2]: 2,
};
globalThis.assert.strictEqual(object[foo1], 1);
globalThis.assert.strictEqual(object[foo2], 2);

export { }