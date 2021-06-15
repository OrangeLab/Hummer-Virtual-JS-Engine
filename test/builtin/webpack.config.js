module.exports = {
  entry: {
    assert: './src/assert.ts',
    '2_function_arguments': './src/2_function_arguments.ts',
    '3_callbacks': './src/3_callbacks.ts',
    '4_object_factory': './src/4_object_factory.ts',
    '5_function_factory': './src/5_function_factory.ts',
    test_array: './src/test_array.ts',
    test_constructor_test: './src/test_constructor_test.ts',
    test_constructor_test2: './src/test_constructor_test2.ts',
    test_conversions: './src/test_conversions.ts',
    test_error: './src/test_error.ts',
    test_exception: './src/test_exception.ts',
    test_function: './src/test_function.ts',
    test_general_test: './src/test_general_test.ts',
    test_general_testGlobals: './src/test_general_testGlobals.ts',
    test_general_testInstanceOf: './src/test_general_testInstanceOf.ts',
    test_general_testNapiRun: './src/test_general_testNapiRun.ts',
    test_general_testNapiStatus: './src/test_general_testNapiStatus.ts',
    test_handle_scope: './src/test_handle_scope.ts',
    test_new_target: './src/test_new_target.ts',
    test_number: './src/test_number.ts',
    test_object: './src/test_object.ts',
    test_promise: './src/test_promise.ts',
    test_properties: './src/test_properties.ts',
    test_string: './src/test_string.ts',
    test_symbol_test1: './src/test_symbol_test1.ts',
    test_symbol_test2: './src/test_symbol_test2.ts',
    test_symbol_test3: './src/test_symbol_test3.ts',
  },
  mode: 'production',
  module: {
    rules: [{
      test: /\.ts$/,
      exclude: /node_modules/,
      use: {
        loader: 'babel-loader',
      },
    }],
  },
  devtool: 'hidden-source-map',
  bail: true,
  resolve: {
    extensions: ['.ts', '...']
  }
};
