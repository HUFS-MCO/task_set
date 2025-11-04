~/task_set/rt-kube$ sudo docker build -t ghcr.io/hufs-mco/rt-kube-task:equal . --push

watch kubectl describe monitorings.rt.francescol96.univr

k delete monitorings.rt.francescol96.univr --all

~/task_set/rt-kube$ k delete -f k8s-eq-task.yaml

~/.local/state/k9s/screen-dumps/kubernetes/kubernetes-admin@kubernetes$ 