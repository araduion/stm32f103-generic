#Owner = Antonescu Radu-Ion
#Year = 2016

#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

PLATFORM=STM32F10X_MD
#CFLAGS=-g -c -D$(PLATFORM) -DCPU_FREQ=48000000 -DUSE_PLL -mthumb -mcpu=cortex-m3
CFLAGS=-g -c -D$(PLATFORM) -DCPU_FREQ=8000000 -mthumb -mcpu=cortex-m3
LDFLAGS=-static
TARGET=arm-none-eabi

SRCS=main.c common.c timer.c uart.c task.c

OBJS = $(SRCS:.c=.o) startup.o

main.bin : main.elf
	$(TARGET)-objcopy -O binary main.elf main.bin

main.elf : $(OBJS) startup.o stm32f103.ld
	$(TARGET)-ld $(LDFLAGS) -Tstm32f103.ld -o main.elf $(OBJS)

.c.o:
	$(TARGET)-gcc $(CFLAGS) $< -o $@

startup.o : startup.s
	$(TARGET)-as -mthumb -mcpu=cortex-m3 -globalize-symbols -g startup.s -o startup.o

objdump : main.elf
	$(TARGET)-objdump -t -l -S -d main.elf > objdump.txt

clean :
	rm -rf *~ ; rm *.o ; rm *.elf ; rm *.bin ; rm -f objdump.txt
