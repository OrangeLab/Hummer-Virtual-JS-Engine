globalThis.addon.NewScope();

globalThis.assert.ok(globalThis.addon.NewScopeEscape() instanceof Object);

globalThis.addon.NewScopeEscapeTwice();

globalThis.assert.throws(
  () => {
    globalThis.addon.NewScopeWithException(() => { throw new RangeError(); });
  },
//   RangeError
);

export { }