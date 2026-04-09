NFV_CNI_CONTROLPLANE_HOST ?= localhost
NFV_CNI_CONTROLPLANE_PORT ?= 8080
NFV_CNI_DATAPLANE_GRPC_HOST ?= localhost
NFV_CNI_DATAPLANE_GRPC_PORT ?= 9090
NFV_CNI_KIND_CLUSTER ?= nfvcni-e2e
NFV_CNI_IMAGE ?= nfvcni:latest

export NFV_CNI_CONTROLPLANE_HOST
export NFV_CNI_CONTROLPLANE_PORT
export NFV_CNI_DATAPLANE_GRPC_HOST
export NFV_CNI_DATAPLANE_GRPC_PORT
export NFV_CNI_KIND_CLUSTER
export NFV_CNI_IMAGE

.PHONY: e2e-setup
e2e-setup: ## Install e2e test dependencies
	go install github.com/onsi/ginkgo/v2/ginkgo@latest

.PHONY: e2e
e2e: ## Run all e2e tests
	ginkgo -v -r test/e2e

.PHONY: e2e-controlplane
e2e-controlplane: ## Run controlplane tests
	ginkgo -v -r test/e2e/controlplane

.PHONY: e2e-dataplane
e2e-dataplane: ## Run dataplane tests
	ginkgo -v -r test/e2e/dataplane

.PHONY: e2e-clean
e2e-clean: ## Clean e2e test environment
	kind delete cluster --name $(NFV_CNI_KIND_CLUSTER) || true