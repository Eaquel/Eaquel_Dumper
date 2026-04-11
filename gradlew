#!/bin/sh

APP_BASE_NAME=$(basename "$0")
DEFAULT_JVM_OPTS='"-Xmx64m" "-Xms64m"'

die() {
    echo "ERROR: $*"
    exit 1
}

cd "$(dirname "$0")" || die "Failed to change directory"

WRAPPER_JAR="Gradle/gradle-wrapper.jar"

[ -f "$WRAPPER_JAR" ] || die "gradle-wrapper.jar not found at $WRAPPER_JAR"

if [ -n "$JAVA_HOME" ]; then
    JAVA="$JAVA_HOME/bin/java"
    [ -x "$JAVA" ] || die "JAVA_HOME points to invalid JDK: $JAVA_HOME"
else
    JAVA=$(which java 2>/dev/null) || die "No java found in PATH. Install JDK 21."
fi

exec "$JAVA" $DEFAULT_JVM_OPTS \
    -classpath "$WRAPPER_JAR" \
    org.gradle.wrapper.GradleWrapperMain \
    "$@"
