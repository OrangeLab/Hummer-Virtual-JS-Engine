module.exports = {
  entry: {
    general: './src/general.ts',
    typeof: './src/typeof.ts',
    conversion: './src/conversion.ts',
    object: './src/object.ts',
    callable: './src/callable.ts'
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
