const array = [
  1,
  9,
  48,
  13493,
  9459324,
  { name: 'hello' },
  [
    'world',
    'node',
    'abi',
  ],
];

globalThis.assert.throws(
  () => {
    globalThis.addon.TestGetElement(array, array.length + 1);
  },
);

globalThis.assert.throws(
  () => {
    globalThis.addon.TestGetElement(array, -2);
  },
);

array.forEach((element, index) => {
  globalThis.assert.strictEqual(globalThis.addon.TestGetElement(array, index), element);
});

// 未实现 deepStrictEqual
// globalThis.assert.deepStrictEqual(globalThis.addon.New(array), array);

globalThis.assert(globalThis.addon.TestHasElement(array, 0));
globalThis.assert.strictEqual(globalThis.addon.TestHasElement(array, array.length + 1), false);

globalThis.assert(globalThis.addon.NewWithLength(0) instanceof Array);
globalThis.assert(globalThis.addon.NewWithLength(1) instanceof Array);
// Check max allowed length for an array 2^32 -1
globalThis.assert(globalThis.addon.NewWithLength(4294967295) instanceof Array);

{
  // Verify that array elements can be deleted.
  const arr = ['a', 'b', 'c', 'd'];

  globalThis.assert.strictEqual(arr.length, 4);
  globalThis.assert.strictEqual(2 in arr, true);
  globalThis.assert.strictEqual(globalThis.addon.TestDeleteElement(arr, 2), true);
  globalThis.assert.strictEqual(arr.length, 4);
  globalThis.assert.strictEqual(2 in arr, false);
}

export { }