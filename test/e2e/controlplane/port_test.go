package controlplane

import (
	"bytes"
	"encoding/json"

	"github.com/breeve/nfvcni/test/e2e/framework"
	types "github.com/breeve/nfvcni/pkg/controlplane/types"

	"github.com/onsi/ginkgo/v2"
	"github.com/onsi/gomega"
)

var _ = ginkgo.Describe("ControlPlane Port Management", func() {
	var cfg *framework.Config
	var client *framework.HTTPClient

	ginkgo.BeforeEach(func() {
		cfg = framework.GetConfig()
		client = framework.NewHTTPClient(cfg)
	})

	ginkgo.AfterEach(func() {
		client.Delete("/admin/ports/test-port")
		client.Delete("/admin/ports/test-port-l2")
		client.Delete("/admin/ports/test-port-l3")
		client.Delete("/admin/ports/test-port-vlanif")
	})

	ginkgo.Describe("POST /admin/ports", func() {
		ginkgo.It("should create a new port with L2 mode", func() {
			port := &types.PortConfig{
				Name: "test-port-l2",
				Mode: types.InterfaceModeLayer2,
				L2: &types.L2PortConfig{
					VlanMode: types.L2VlanModeAccess,
					VlanID:   10,
				},
			}
			body, _ := json.Marshal(port)
			resp, err := client.PostWithBody("/admin/ports", bytes.NewReader(body))
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(201))
			resp.Body.Close()
		})

		ginkgo.It("should create a new port with L3 mode", func() {
			port := &types.PortConfig{
				Name: "test-port-l3",
				Mode: types.InterfaceModeLayer3,
			}
			body, _ := json.Marshal(port)
			resp, err := client.PostWithBody("/admin/ports", bytes.NewReader(body))
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(201))
			resp.Body.Close()
		})

		ginkgo.It("should create a new port with VlanIF mode", func() {
			port := &types.PortConfig{
				Name: "test-port-vlanif",
				Mode: types.InterfaceModeVlanIF,
				L2: &types.L2PortConfig{
					VlanMode:    types.L2VlanModeTrunk,
					StartVlanID: 100,
					EndVlanID:   200,
				},
			}
			body, _ := json.Marshal(port)
			resp, err := client.PostWithBody("/admin/ports", bytes.NewReader(body))
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(201))
			resp.Body.Close()
		})

		ginkgo.It("should return 409 for duplicate port", func() {
			port := &types.PortConfig{
				Name: "test-port",
				Mode: types.InterfaceModeLayer2,
			}
			body, _ := json.Marshal(port)
			client.PostWithBody("/admin/ports", bytes.NewReader(body))
			resp, err := client.PostWithBody("/admin/ports", bytes.NewReader(body))
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(409))
			resp.Body.Close()
		})
	})

	ginkgo.Describe("GET /admin/ports", func() {
		ginkgo.It("should return port list", func() {
			resp, err := client.Get("/admin/ports")
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(200))
			resp.Body.Close()
		})
	})

	ginkgo.Describe("GET /admin/ports/{name}", func() {
		ginkgo.It("should return the port", func() {
			port := &types.PortConfig{
				Name: "test-port",
				Mode: types.InterfaceModeLayer2,
			}
			body, _ := json.Marshal(port)
			client.PostWithBody("/admin/ports", bytes.NewReader(body))

			resp, err := client.Get("/admin/ports/test-port")
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(200))
			resp.Body.Close()
		})

		ginkgo.It("should return 404 for non-existent port", func() {
			resp, err := client.Get("/admin/ports/non-existent")
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(404))
			resp.Body.Close()
		})
	})

	ginkgo.Describe("DELETE /admin/ports/{name}", func() {
		ginkgo.It("should delete the port", func() {
			port := &types.PortConfig{
				Name: "test-port",
				Mode: types.InterfaceModeLayer2,
			}
			body, _ := json.Marshal(port)
			client.PostWithBody("/admin/ports", bytes.NewReader(body))

			resp, err := client.Delete("/admin/ports/test-port")
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(204))
			resp.Body.Close()
		})
	})
})