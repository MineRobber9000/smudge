


int main(void) {
    if (spawn_world(&world) != 0) {
        fprintf(stderr, "Error (pthread)");
        exit(1);
    }
    start_server(PORT);
    return 0;
}
