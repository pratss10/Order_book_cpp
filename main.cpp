#include <iostream>
#include <map> 
#include <set>
#include <list>
#include <cmath>
#include <ctime>
#include <deque>
#include <queue>
#include <stack>
#include <limits>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <variant>
#include <tuple>
#include <stdexcept>
#include <format>
#include <optional>

using namespace std;

enum class OrderType{
    GoodTillCancel,
    FillAndKill
};

enum class Side{
    Buy,
    Sell
};

#define Price int
#define Quantity uint32_t
#define OrderId uint64_t

struct LevelInfo{
    Price price_;
    Quantity quantity_ ;
};

using  LevelInfos = vector<LevelInfo>;

class OrderBookLevelInfos{

private:
    LevelInfos bids_;
    LevelInfos asks_;
    
public:
    OrderBookLevelInfos(const LevelInfos &bids, const LevelInfos& asks)
        : bids_ {bids}
        , asks_ {asks}
    {}

    const LevelInfos& GetBids() const { return bids_ ; }
    const LevelInfos& GetAsks() const { return asks_ ; }
};

class Order{
    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;
    
    public:
        Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity):
            orderType_ { orderType }
            , orderId_ { orderId }
            , side_ { side}
            , price_ { price }
            , initialQuantity_ { quantity }
            , remainingQuantity_ { quantity }
        {}

        OrderId GetOrderId() const { return orderId_ ; }
        Side GetSide() const { return side_ ; }
        Price GetPrice() const { return price_ ; }
        OrderType GetOrderType() const { return orderType_ ; }
        Quantity GetInitialQuantity() const { return initialQuantity_ ; }
        Quantity GetRemainingQuantity() const { return remainingQuantity_ ; }
        Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }
        bool IsFilled() const { return GetRemainingQuantity()==0; }
        void Fill(Quantity quantity){
            if (quantity > GetRemainingQuantity())
                throw logic_error(format("Order ({}) cannot be filled for more than its remaining quantity.",GetOrderId()));
            remainingQuantity_ -= quantity;
        }
};

using OrderPointer = shared_ptr<Order>;
using OrderPointers = list<OrderPointer>;

class OrderModify{
private:
    OrderId orderId_;
    Price price_;
    Side side_;
    Quantity quantity_;
    
public: 
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
    : orderId_ {orderId}
    , price_ {price}
    , side_ {side}
    , quantity_ {quantity}
    {}

    OrderId GetOrderId() const { return orderId_ ; }
    Price GetPrice() const { return price_ ; }
    Side GetSide() const { return side_ ;}
    Quantity GetQuantity() const { return quantity_; }

    OrderPointer ToOrderPointer(OrderType type)const{
        return  make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity() );
    }
};

struct TradeInfo{
    OrderId orderId_;
    Price price_;
    Quantity quantity_;
};

class Trade{
private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
public:
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
        : bidTrade_ { bidTrade}
        , askTrade_ { askTrade}
    {}

    const TradeInfo& GetBidTrade() const { return bidTrade_; }
    const TradeInfo& GetAskTrade() const { return askTrade_; }
};

using Trades = vector<Trade>;

class OrderBook{
private:

    struct OrderEntry{
        OrderPointer order_{ nullptr };
        OrderPointers::iterator location_;
    };

    map<Price, OrderPointers, greater<Price>> bids_;
    map<Price, OrderPointers, less<Price>> asks_;
    unordered_map<OrderId, OrderEntry> orders_;

    bool CanMatch(Side side, Price price) const{
        if(side == Side::Buy){
            if(asks_.empty())
                return false;

            const auto& [bestAsk, _] = *asks_.begin();
            return price >= bestAsk;
        }
        else{
            if(bids_.empty())
                return false;
            
            const auto& [bestBid, _] = *bids_.begin();
            return price <= bestBid ;
        }
    }

    Trades MatchOrders(){
        Trades trades;
        trades.reserve(orders_.size());

        while(true){
            if(bids_.empty() || asks_.empty())
                break;

            auto& [bidPrice, bids] = *bids_.begin();
            auto& [askPrice, asks] = *asks_.begin();

            if(bidPrice < askPrice)
                break;
            
            while(bids.size() && asks.size()){
                auto& bid = bids.front();
                auto& ask = asks.front();

                Quantity quantity = min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

                bid->Fill(quantity);
                ask->Fill(quantity);

                if(bid->IsFilled()){
                    bids.pop_front();
                    orders_.erase(bid->GetOrderId());
                }

                if(ask->IsFilled()){
                    asks.pop_front();
                    orders_.erase(ask->GetOrderId());
                }

                if(bids.empty())
                    bids_.erase(bidPrice);

                if(asks.empty())
                    asks_.erase(askPrice);

                trades.push_back(Trade{
                    TradeInfo { bid->GetOrderId(), bid->GetPrice(), quantity},
                    TradeInfo { ask->GetOrderId(), ask->GetPrice(), quantity }
                    });
            }
        }
        if(!bids_.empty()){
            auto& [_, bids] = *bids_.begin();
            auto& order = bids.front();
            if(order->GetOrderType()== OrderType::FillAndKill)
                CancelOrder(order->GetOrderId());
        }
        if(!asks_.empty()){
            auto& [_, asks] = *asks_.begin();
            auto& order = asks.front();
            if(order->GetOrderType()== OrderType::FillAndKill)
                CancelOrder(order->GetOrderId());
        }

        return trades;
    }

public:

    Trades AddOrder(OrderPointer order){
        if(orders_.contains(order->GetOrderId()))
            return {};
        
        if(order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(),order->GetPrice()))
            return {};

        OrderPointers::iterator iterator;

        if(order->GetSide()==Side::Buy){
            auto& orders = bids_[order->GetPrice()];
            orders.push_back(order);
            iterator = next(orders.begin(), orders.size()-1);
        }
        else{
            auto& orders = asks_[order->GetPrice()];
            orders.push_back(order);
            iterator = next(orders.begin(), orders.size()-1);
        }

        orders_.insert({ order->GetOrderId(), OrderEntry{ order,iterator}});
        return MatchOrders();
    }

    void CancelOrder(OrderId orderId){
        if(!orders_.contains(orderId))
            return;

        const auto& [order,orderIterator] = orders_.at(orderId);
        orders_.erase(orderId);

        if(order->GetSide() == Side::Sell){
            auto price = order->GetPrice();
            auto& orders = asks_.at(price);
            orders.erase(orderIterator);

            if(orders.empty())
                asks_.erase(price);
        }
        else{
            auto price = order->GetPrice();
            auto& orders = bids_.at(price);
            orders.erase(orderIterator);

            if(orders.empty())
                bids_.erase(price);
        }
    }

    Trades MatchOrder(OrderModify order){
        if(!orders_.contains(order.GetOrderId()))
            return {};

        const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
        CancelOrder(order.GetOrderId());
        return AddOrder(order.ToOrderPointer(existingOrder->GetOrderType()));
    }

    size_t Size()const { return orders_.size(); }

    OrderBookLevelInfos GetOrderInfos() const{
        LevelInfos bidInfos, askInfos;
        bidInfos.reserve(orders_.size());
        askInfos.reserve(orders_.size());

        auto CreateLevelInfos = [](Price price,const OrderPointers& orders){
            return LevelInfo{ price, accumulate(orders.begin(),orders.end(), (Quantity)0,
            [](Quantity runningSum, const OrderPointer& order)
            { return runningSum + order->GetRemainingQuantity();}) };
        };

        for(const auto& [price, orders] : bids_)
            bidInfos.push_back(CreateLevelInfos(price,orders));

        for(const auto& [price,orders] : asks_)
            askInfos.push_back(CreateLevelInfos(price,orders));
        
        return OrderBookLevelInfos{bidInfos, askInfos};
    }
};

int main(){

    // testing it
    OrderBook orderbook;
    const OrderId orderId = 1;
    orderbook.AddOrder(make_shared<Order>(OrderType::GoodTillCancel,orderId, Side::Buy, 100,10));
    cout << orderbook.Size() << "\n";
    orderbook.CancelOrder(orderId);
    cout << orderbook.Size() << "\n";

    return 0;
}