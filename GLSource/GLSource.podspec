Pod::Spec.new do |s|
    s.name          = "GLSource"

    s.authors        = "Henry"
    s.homepage       = "http://henry.beer"
    s.license        = 'Private'
    s.summary        = s.name
    s.source         = {:path => '.'}
    s.version        = "1.0"

    s.frameworks = 'OpenGL', 'GLUT'

    s.source_files  = "include/**/*.{h,m}"
    s.public_header_files  = "include/**/*.h"
    s.vendored_libraries = "libGLTools.a"
end
