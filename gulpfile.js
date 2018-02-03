var gulp = require('gulp');
var htmlmin = require('gulp-htmlmin');
var replace = require('gulp-string-replace');
var clean = require('gulp-clean');
var fs = require("fs");

gulp.task('clean', function() {
    return gulp.src('dist/*')
        .pipe(clean());
});

var options = {
    collapseWhitespace: true,
    minifyJS: true,
    minifyCSS: true
};
gulp.task('minify', ['clean'], function() {
    return gulp.src('src/*.html')
        .pipe(htmlmin(options))
        .pipe(gulp.dest('dist'));
});

gulp.task('replace', ['minify'], function() {
    gulp.src(['dist/index.html'])
        .pipe(replace('"', "'"))
        .pipe(gulp.dest('dist/'))
        .on('end', function() {
            var fileContent = fs.readFileSync('dist/index.html', 'utf8');
            gulp.src(['src/server/server.ino'])
                .pipe(replace('INDEX', fileContent))
                .pipe(gulp.dest('dist/server'))
        })
});

gulp.task('default', [
    'clean',
    'minify',
    'replace'
]);