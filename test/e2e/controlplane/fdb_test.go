package controlplane

import (
	"github.com/breeve/nfvcni/test/e2e/framework"

	"github.com/onsi/ginkgo/v2"
	"github.com/onsi/gomega"
)

var _ = ginkgo.Describe("ControlPlane FDB", func() {
	var cfg *framework.Config
	var client *framework.HTTPClient

	ginkgo.BeforeEach(func() {
		cfg = framework.GetConfig()
		client = framework.NewHTTPClient(cfg)
	})

	ginkgo.Describe("GET /admin/fdb", func() {
		ginkgo.It("should return FDB table", func() {
			resp, err := client.Get("/admin/fdb")
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(200))
			resp.Body.Close()
		})

		ginkgo.It("should return JSON array", func() {
			resp, err := client.Get("/admin/fdb")
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			contentType := resp.Header.Get("Content-Type")
			gomega.Expect(contentType).To(gomega.ContainSubstring("application/json"))
			resp.Body.Close()
		})
	})
})