#!/bin/sh
set -e

TASK="$1"
shift || true

case "$TASK" in
	planner)
		CMD="/app/planner"
		;;
	ekf)
		CMD="/app/ekf"
		;;
	localization)
		CMD="/app/localization"
		;;
	lidar)
		CMD="/app/lidar"
		;;
	""|--help|-h|help)
		echo "Usage: <planner|ekf|localization|lidar> [args...]";
		exit 0
		;;
	*)
		echo "Unknown task: $TASK" >&2
		echo "Usage: <planner|ekf|localization|lidar> [args...]" >&2
		exit 1
		;;
esac

# Run selected command without replacing the shell, capture exit code
set +e
"$CMD" "$@"
EXIT_CODE=$?
set -e

echo "[$TASK] exited with code $EXIT_CODE. Entering idle mode..."

# If KEEP_ALIVE=bash, open a bash shell; otherwise sleep forever
if [ "$KEEP_ALIVE" = "bash" ] && command -v bash >/dev/null 2>&1; then
	exec /bin/bash -l
else
	# Portable idle loop
	while true; do sleep 3600; done
fi
