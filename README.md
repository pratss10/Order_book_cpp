# Order Book System

## Overview

This project implements a basic Order Book System for managing and processing buy and sell orders in a financial trading system. The system includes functionalities for adding, modifying, and canceling orders, as well as matching orders based on the best available prices.

## Features

- **Order Types**: Supports `GoodTillCancel` and `FillAndKill` order types.
- **Order Sides**: Handles both `Buy` and `Sell` sides.
- **Order Matching**: Automatically matches buy and sell orders based on price.
- **Order Management**: Allows adding, modifying, and canceling orders.
- **Order Book Information**: Provides information about current bids and asks.

## Getting Started

### Prerequisites

- A C++ compiler supporting C++20 (e.g., `g++`).
- Basic understanding of C++ and financial trading concepts.

### Building the Project

To build the project, use the following command:

```bash
g++ -fdiagnostics-color=always -g -std=c++20 main.cpp -o order_book
```

## Code Structure

The project is comprised of a single file, `main.cpp`, which includes the following components:

- **OrderBook**: Manages the order book, handles order matching and cancellation.
- **Order**: Represents an individual order, characterized by properties such as type, side, price, and quantity.
- **OrderModify**: Represents modifications made to an existing order.
- **Trade**: Represents a trade that occurs between a buy and sell order.
- **OrderBookLevelInfos**: Provides information about the current levels of bids and asks.


##Classes and Structures

-OrderBook: Handles the addition, modification, and cancellation of orders, and performs order matching. Uses bids_ and asks_ to maintain order levels, and orders_ to keep track of active orders.

-Order: Represents a single order. Contains details such as order type, ID, side, price, initial and remaining quantity. Provides methods to fill the order and check its status.

-OrderModify: Represents a modification to an existing order. Includes methods to convert it to an Order object for processing.

-Trade: Represents a trade between a buy order and a sell order. Contains details such as order ID, price, and quantity traded.

-OrderBookLevelInfos: Contains the bid and ask levels of the order book. Provides a way to retrieve current market depth.
