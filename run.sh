#!/bin/sh
set -e

TASK="$1"
shift || true

case "$TASK" in
	planner)
		exec /app/planner "$@"
		;;
	ekf)
		exec /app/ekf "$@"
		;;
	localization)
		exec /app/localization "$@"
		;;
	lidar)
		exec /app/lidar "$@"
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
