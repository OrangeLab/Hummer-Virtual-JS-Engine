/* eslint-disable @typescript-eslint/no-empty-function */
// We can only perform this test if we have a working Symbol.hasInstance
if (typeof Symbol !== 'undefined' && 'hasInstance' in Symbol
  && typeof Symbol.hasInstance === 'symbol') {
  // eslint-disable-next-line no-inner-declarations
  function compareToNative(theObject: unknown, theConstructor: () => void) {
    globalThis.assert.strictEqual(
      globalThis.addon.doInstanceOf(theObject, theConstructor),
      (theObject instanceof theConstructor),
    );
  }

  // eslint-disable-next-line no-inner-declarations
  function MyClass() { }
  Object.defineProperty(MyClass, Symbol.hasInstance, {
    value(candidate: Record<string, unknown>) {
      return 'mark' in candidate;
    },
  });

  // eslint-disable-next-line no-inner-declarations
  function MySubClass() { }
  MySubClass.prototype = new MyClass();

  let x = new MySubClass();
  let y = new MySubClass();
  x.mark = true;

  compareToNative(x, MySubClass);
  compareToNative(y, MySubClass);
  compareToNative(x, MyClass);
  compareToNative(y, MyClass);

  x = new MyClass();
  y = new MyClass();
  x.mark = true;

  compareToNative(x, MySubClass);
  compareToNative(y, MySubClass);
  compareToNative(x, MyClass);
  compareToNative(y, MyClass);
}

export { }