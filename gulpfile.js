var gulp = require('gulp');
var htmlmin = require('gulp-htmlmin');

var options = {
    collapseWhitespace: true,
    minifyJS: true,
    minifyCSS: true
};
gulp.task('minify', function() {
    return gulp.src('src/*.html')
        .pipe(htmlmin(options))
        .pipe(gulp.dest('dist'));
});