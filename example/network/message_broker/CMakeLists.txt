cmake_minimum_required(VERSION 3.24)

project(network_message_broker_example_project)

add_subdirectory(broker)

add_subdirectory(pub)
add_subdirectory(sub)

add_subdirectory(pub-reflect)
add_subdirectory(sub-reflect)




