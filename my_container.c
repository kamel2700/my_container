#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#define STACK_SIZE (1024 * 1024)

static char child_stack[STACK_SIZE];

static int child_fn() {
    printf("Clone internal PID: %ld\n", (long) getpid());
    pid_t child_pid = fork();
    if (child_pid) {
        printf("Clone fork child PID: %ld\n", (long) child_pid);
        //Mount namespace
        system("mount --make-rprivate -o remount /");
        system("mount proc /proc -t proc --make-private");
        //Up new network interface
        printf("\nGuest network namespace:\n");
        system("ifconfig veth1 10.1.1.2/24 up");
        system("ip link");
        printf("\n\n");
        //Loop
        system("dd if=/dev/zero of=image.fs count=10 bs=1048576");
        system("losetup /dev/loop7 image.fs");
        system("mkfs.ext4 image.fs");
        system("mount -t ext4 /dev/loop7 /home --make-private");
        system("dd if=/dev/zero of=/home/check_file.txt count=1 bs=1024000");
        printf("Files in the new mountspace /home:\n");
        system("ls /home");

        // UNCOMMENT to test my container
        // system("sysbench --test=cpu --cpu-max-prime=20000 run");
        // system("sysbench --test=fileio --file-total-size=40G prepare");
        // system("sysbench --test=fileio --file-total-size=40G --file-test-mode=rndrw --init-rng=on --max-time=300 --max-requests=0 run");
        // system("sysbench --test=fileio --file-total-size=40G cleanup");

        //Enter bash
        system("bash");
    }
    return EXIT_SUCCESS;
}

int main() {
    pid_t child_pid = clone(child_fn, child_stack + STACK_SIZE, CLONE_NEWPID | SIGCHLD | CLONE_NEWNET | CLONE_NEWNS,
                            NULL);
    printf("clone() = %ld\n", (long) child_pid);

    // UNCOMMENT to bind child process with CGroups
    // sprintf(buf, "echo %d > /sys/fs/cgroup/cpu/demo/tasks", child_pid);
    // system(buf);

    //Configure network interfaces
    char newnt[1024 * sizeof(char)];
    sprintf(newnt, "ip link add name veth0 type veth peer name veth1 netns %ld", (long) child_pid);
    system(newnt);
    system("ifconfig veth0 10.1.1.1/24 up");
    printf("\nHost network namespace:\n");
    system("ip link");
    waitpid(child_pid, NULL, 0);
    //Check that check_file.txt is not in the host
    printf("Files on the local machine:\n");
    system("ls");
    return EXIT_SUCCESS;
}
