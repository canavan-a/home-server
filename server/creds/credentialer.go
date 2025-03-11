package credentialer

import (
	"strconv"
	"sync"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
)

type Credentialer struct {
	Map   map[string]int
	Mutex sync.Mutex
}

func NewCreds() (c *Credentialer) {
	return &Credentialer{
		Map:   make(map[string]int),
		Mutex: sync.Mutex{},
	}
}

func (c *Credentialer) CreateEntry(count int) (credential string) {
	credential = uuid.NewString()
	c.Mutex.Lock()
	c.Map[credential] = count
	c.Mutex.Unlock()

	return credential
}

func (c *Credentialer) ValidateDecrement(cred string) bool {
	c.Mutex.Lock()
	defer c.Mutex.Unlock()

	value, exists := c.Map[cred]

	if !exists {
		return false
	}

	if value == 0 {
		delete(c.Map, cred)
		return false
	}

	c.Map[cred] = value - 1

	return true

}

func (c *Credentialer) CreateEntryHandler() func(*gin.Context) {
	return func(context *gin.Context) {
		count := context.Query("count")
		countNumber, err := strconv.Atoi(count)
		if err != nil {
			context.JSON(400, "invalid count supplied")
			return
		}
		cred := c.CreateEntry(countNumber)
		context.JSON(200, cred)
	}
}

func (c *Credentialer) CreateMiddleware() func(*gin.Context) {
	return func(context *gin.Context) {
		cred := context.Query("cred")
		validated := c.ValidateDecrement(cred)
		if !validated {
			context.Abort()
			return
		}
		context.Next()
	}
}
