const object = {
  hello: 'world',
  array: [
    1, 94, 'str', 12.321, { test: 'obj in arr' },
  ],
  newObject: {
    test: 'obj in obj',
  },
};

globalThis.assert.strictEqual(globalThis.addon.Get(object, 'hello'), 'world');
globalThis.assert.strictEqual(globalThis.addon.GetNamed(object, 'hello'), 'world');
// globalThis.assert.deepStrictEqual(globalThis.addon.Get(object, 'array'),
//   [1, 94, 'str', 12.321, { test: 'obj in arr' }]);
// globalThis.assert.deepStrictEqual(globalThis.addon.Get(object, 'newObject'),
//   { test: 'obj in obj' });

globalThis.assert(globalThis.addon.Has(object, 'hello'));
globalThis.assert(globalThis.addon.HasNamed(object, 'hello'));
globalThis.assert(globalThis.addon.Has(object, 'array'));
globalThis.assert(globalThis.addon.Has(object, 'newObject'));

const newObject = globalThis.addon.New();
globalThis.assert(globalThis.addon.Has(newObject, 'test_number'));
globalThis.assert.strictEqual(newObject.test_number, 987654321);
globalThis.assert.strictEqual(newObject.test_string, 'test string');

{
  // Verify that napi_get_property() walks the prototype chain.
  // eslint-disable-next-line no-inner-declarations
  function MyObject() {
    this.foo = 42;
    this.bar = 43;
  }

  MyObject.prototype.bar = 44;
  MyObject.prototype.baz = 45;

  const obj = new MyObject();

  globalThis.assert.strictEqual(globalThis.addon.Get(obj, 'foo'), 42);
  globalThis.assert.strictEqual(globalThis.addon.Get(obj, 'bar'), 43);
  globalThis.assert.strictEqual(globalThis.addon.Get(obj, 'baz'), 45);
  globalThis.assert.strictEqual(globalThis.addon.Get(obj, 'toString'),
    Object.prototype.toString);
}

{
  // Verify that napi_has_own_property() fails if property is not a name.
  // eslint-disable-next-line @typescript-eslint/no-empty-function
  [true, false, null, undefined, {}, [], 0, 1, () => { }].forEach((value) => {
    globalThis.assert.throws(() => {
      globalThis.addon.HasOwn({}, value);
    },
      // /^Error: A string or symbol was expected$/
    );
  });
}

{
  // Verify that napi_has_own_property() does not walk the prototype chain.
  const symbol1 = Symbol();
  const symbol2 = Symbol();

  // eslint-disable-next-line no-inner-declarations
  function MyObject() {
    this.foo = 42;
    this.bar = 43;
    this[symbol1] = 44;
  }

  MyObject.prototype.bar = 45;
  MyObject.prototype.baz = 46;
  MyObject.prototype[symbol2] = 47;

  const obj = new MyObject();

  globalThis.assert.strictEqual(globalThis.addon.HasOwn(obj, 'foo'), true);
  globalThis.assert.strictEqual(globalThis.addon.HasOwn(obj, 'bar'), true);
  globalThis.assert.strictEqual(globalThis.addon.HasOwn(obj, symbol1), true);
  globalThis.assert.strictEqual(globalThis.addon.HasOwn(obj, 'baz'), false);
  globalThis.assert.strictEqual(globalThis.addon.HasOwn(obj, 'toString'), false);
  globalThis.assert.strictEqual(globalThis.addon.HasOwn(obj, symbol2), false);
}

{
  // globalThis.addon.Inflate increases all properties by 1
  //   const cube = {
  //     x: 10,
  //     y: 10,
  //     z: 10,
  //   };

  //   globalThis.assert.deepStrictEqual(globalThis.addon.Inflate(cube), { x: 11, y: 11, z: 11 });
  //   globalThis.assert.deepStrictEqual(globalThis.addon.Inflate(cube), { x: 12, y: 12, z: 12 });
  //   globalThis.assert.deepStrictEqual(globalThis.addon.Inflate(cube), { x: 13, y: 13, z: 13 });
  //   cube.t = 13;
  //   globalThis.assert.deepStrictEqual(
  //     globalThis.addon.Inflate(cube), {
  //       x: 14, y: 14, z: 14, t: 14,
  //     },
  //   );

  //   const sym1 = Symbol('1');
  //   const sym2 = Symbol('2');
  //   const sym3 = Symbol('3');
  //   const sym4 = Symbol('4');
  const object2 = {
    // [sym1]: '@@iterator',
    // [sym2]: sym3,
  };

  //   globalThis.assert(globalThis.addon.Has(object2, sym1));
  //   globalThis.assert(globalThis.addon.Has(object2, sym2));
  //   globalThis.assert.strictEqual(globalThis.addon.Get(object2, sym1), '@@iterator');
  //   globalThis.assert.strictEqual(globalThis.addon.Get(object2, sym2), sym3);
  globalThis.assert(globalThis.addon.Set(object2, 'string', 'value'));
  globalThis.assert(globalThis.addon.SetNamed(object2, 'named_string', 'value'));
  //   globalThis.assert(globalThis.addon.Set(object2, sym4, 123));
  globalThis.assert(globalThis.addon.Has(object2, 'string'));
  globalThis.assert(globalThis.addon.HasNamed(object2, 'named_string'));
  //   globalThis.assert(globalThis.addon.Has(object2, sym4));
  globalThis.assert.strictEqual(globalThis.addon.Get(object2, 'string'), 'value');
  //   globalThis.assert.strictEqual(globalThis.addon.Get(object2, sym4), 123);
}

{
  // Wrap a pointer in a JS object, then verify the pointer can be unwrapped.
  const wrapper = {};
  globalThis.addon.Wrap(wrapper);

  globalThis.assert(globalThis.addon.Unwrap(wrapper));
}

{
  // Verify that wrapping doesn't break an object's prototype chain.
  const wrapper: { protoA?: boolean } = {};
  const protoA = { protoA: true };
  Object.setPrototypeOf(wrapper, protoA);
  globalThis.addon.Wrap(wrapper);

  globalThis.assert(globalThis.addon.Unwrap(wrapper));
  globalThis.assert(wrapper.protoA);
}

// {
//   // Verify the pointer can be unwrapped after inserting in the prototype chain.
//   const wrapper = {};
//   const protoA = { protoA: true };
//   Object.setPrototypeOf(wrapper, protoA);
//   globalThis.addon.Wrap(wrapper);

//   const protoB = { protoB: true };
//   Object.setPrototypeOf(protoB, Object.getPrototypeOf(wrapper));
//   Object.setPrototypeOf(wrapper, protoB);

//   globalThis.assert(globalThis.addon.Unwrap(wrapper));
//   globalThis.assert(wrapper.protoA, true);
//   globalThis.assert(wrapper.protoB, true);
// }

// {
//   // Verify that objects can be type-tagged and type-tag-checked.
//   const obj1 = globalThis.addon.TypeTaggedInstance(0);
//   const obj2 = globalThis.addon.TypeTaggedInstance(1);

//   // Verify that type tags are correctly accepted.
//   globalThis.assert.strictEqual(globalThis.addon.CheckTypeTag(0, obj1), true);
//   globalThis.assert.strictEqual(globalThis.addon.CheckTypeTag(1, obj2), true);

//   // Verify that wrongly tagged objects are rejected.
//   globalThis.assert.strictEqual(globalThis.addon.CheckTypeTag(0, obj2), false);
//   globalThis.assert.strictEqual(globalThis.addon.CheckTypeTag(1, obj1), false);

//   // Verify that untagged objects are rejected.
//   globalThis.assert.strictEqual(globalThis.addon.CheckTypeTag(0, {}), false);
//   globalThis.assert.strictEqual(globalThis.addon.CheckTypeTag(1, {}), false);
// }

{
  // Verify that normal and nonexistent properties can be deleted.
  const sym = Symbol();
  const obj = { foo: 'bar', [sym]: 'baz' };

  globalThis.assert.strictEqual('foo' in obj, true);
  globalThis.assert.strictEqual(sym in obj, true);
  globalThis.assert.strictEqual('does_not_exist' in obj, false);
  globalThis.assert.strictEqual(globalThis.addon.Delete(obj, 'foo'), true);
  globalThis.assert.strictEqual('foo' in obj, false);
  globalThis.assert.strictEqual(sym in obj, true);
  globalThis.assert.strictEqual('does_not_exist' in obj, false);
  //   globalThis.assert.strictEqual(globalThis.addon.Delete(obj, sym), true);
  globalThis.assert.strictEqual('foo' in obj, false);
  //   globalThis.assert.strictEqual(sym in obj, false);
  globalThis.assert.strictEqual('does_not_exist' in obj, false);
}

{
  // Verify that non-configurable properties are not deleted.
  const obj = {};

  Object.defineProperty(obj, 'foo', { configurable: false });
  globalThis.assert.strictEqual(globalThis.addon.Delete(obj, 'foo'), false);
  globalThis.assert.strictEqual('foo' in obj, true);
}

{
  // Verify that prototype properties are not deleted.
  // eslint-disable-next-line no-inner-declarations
  function Foo() {
    this.foo = 'bar';
  }

  Foo.prototype.foo = 'baz';

  const obj = new Foo();

  globalThis.assert.strictEqual(obj.foo, 'bar');
  globalThis.assert.strictEqual(globalThis.addon.Delete(obj, 'foo'), true);
  globalThis.assert.strictEqual(obj.foo, 'baz');
  globalThis.assert.strictEqual(globalThis.addon.Delete(obj, 'foo'), true);
  globalThis.assert.strictEqual(obj.foo, 'baz');
}

// {
//   // Verify that napi_get_property_names gets the right set of property names,
//   // i.e.: includes prototypes, only enumerable properties, skips symbols,
//   // and includes indices and converts them to strings.

//   const object = Object.create({
//     inherited: 1,
//   });

//   const fooSymbol = Symbol('foo');

//   object.normal = 2;
//   object[fooSymbol] = 3;
//   Object.defineProperty(object, 'unenumerable', {
//     value: 4,
//     enumerable: false,
//     writable: true,
//     configurable: true,
//   });
//   object[5] = 5;

//   globalThis.assert.deepStrictEqual(globalThis.addon.GetPropertyNames(object),
//     ['5', 'normal', 'inherited']);

//   globalThis.assert.deepStrictEqual(globalThis.addon.GetSymbolNames(object),
//     [fooSymbol]);
// }

// Verify that passing NULL to napi_set_property() results in the correct
// error.
// globalThis.assert.deepStrictEqual(globalThis.addon.TestSetProperty(), {
//   envIsNull: 'Invalid argument',
//   objectIsNull: 'Invalid argument',
//   keyIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
// });

// Verify that passing NULL to napi_has_property() results in the correct
// error.
// globalThis.assert.deepStrictEqual(globalThis.addon.TestHasProperty(), {
//   envIsNull: 'Invalid argument',
//   objectIsNull: 'Invalid argument',
//   keyIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
// });

// Verify that passing NULL to napi_get_property() results in the correct
// error.
// globalThis.assert.deepStrictEqual(globalThis.addon.TestGetProperty(), {
//   envIsNull: 'Invalid argument',
//   objectIsNull: 'Invalid argument',
//   keyIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
// });

// {
//   const obj = { x: 'a', y: 'b', z: 'c' };

//   globalThis.addon.TestSeal(obj);

//   globalThis.assert.strictEqual(Object.isSealed(obj), true);

//   globalThis.assert.throws(() => {
//     obj.w = 'd';
//   }, /Cannot add property w, object is not extensible/);

//   globalThis.assert.throws(() => {
//     delete obj.x;
//   }, /Cannot delete property 'x' of #<Object>/);

//   // Sealed objects allow updating existing properties,
//   // so this should not throw.
//   obj.x = 'd';
// }

// {
//   const obj = { x: 10, y: 10, z: 10 };

//   globalThis.addon.TestFreeze(obj);

//   globalThis.assert.strictEqual(Object.isFrozen(obj), true);

//   globalThis.assert.throws(() => {
//     obj.x = 10;
//   }, /Cannot assign to read only property 'x' of object '#<Object>/);

//   globalThis.assert.throws(() => {
//     obj.w = 15;
//   }, /Cannot add property w, object is not extensible/);

//   globalThis.assert.throws(() => {
//     delete obj.x;
//   }, /Cannot delete property 'x' of #<Object>/);
// }

export { }