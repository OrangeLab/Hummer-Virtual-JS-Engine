module.exports = {
    entry: './src/index.ts',
    mode: 'development',
    module: {
        rules: [{
            test: /\.ts$/,
            exclude: /node_modules/,
            use: {
                loader: 'babel-loader'
            }
        }]
    },
    resolve: {
        extensions: ['.ts', '...']
    },
    devtool: 'hidden-source-map',
    bail: true
}