// class Class extends globalThis.exports.BaseClass 最终调用 BaseClass 构造函数的时候，是无法获得 Class 的构造函数的
// class Class extends globalThis.addon.BaseClass {
//   constructor() {
//     super();
//     this.method();
//   }

//   method() {
//     this.ok = true;
//   }
// }

// globalThis.assert.ok(new Class() instanceof globalThis.addon.BaseClass);
// globalThis.assert.ok(new Class().ok);
globalThis.assert.ok(globalThis.addon.OrdinaryFunction());
globalThis.assert.ok(
  new globalThis.addon.Constructor(globalThis.addon.Constructor) instanceof globalThis.addon.Constructor,
);

export { }