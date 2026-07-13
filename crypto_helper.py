import aiohttp
import logging
import config

logger = logging.getLogger(__name__)

async def verify_trc20_tx(txid: str, expected_amount: float) -> bool:
    """
    Verifies if a USDT (TRC20) transaction to the Admin's address is valid and has the correct amount.
    """
    if not config.TRONGRID_API_KEY or not config.ADMIN_TRC20_ADDRESS:
        logger.warning("TronGrid API Key or Admin TRC20 address not set. Skipping auto crypto verification.")
        return False
        
    url = f"https://api.trongrid.io/v1/transactions/{txid}/events"
    headers = {
        "TRON-PRO-API-KEY": config.TRONGRID_API_KEY
    }
    
    try:
        async with aiohttp.ClientSession() as session:
            async with session.get(url, headers=headers) as response:
                if response.status != 200:
                    logger.error(f"TronGrid API returned status {response.status}")
                    return False
                
                data = await response.json()
                if not data.get("success") or not data.get("data"):
                    return False
                
                # Check events inside transaction
                for event in data["data"]:
                    # USDT Contract Address: TR7NHqjeKQxGTCi8q8ZY4pL8otSzgjLj6t
                    if event.get("contract_address") == "TR7NHqjeKQxGTCi8q8ZY4pL8otSzgjLj6t" and event.get("event_name") == "Transfer":
                        result = event.get("result", {})
                        
                        to_address = result.get("to")
                        value_str = result.get("value", "0")
                        
                        try:
                            value_usdt = float(value_str) / 1_000_000 # USDT has 6 decimals
                        except ValueError:
                            value_usdt = 0
                            
                        # Compare addresses (TronGrid may return base58 or hex, typically base58 for public APIs)
                        if to_address == config.ADMIN_TRC20_ADDRESS and value_usdt >= expected_amount:
                            return True
                            
                return False
                
    except Exception as e:
        logger.error(f"Auto Crypto Verification failed: {e}")
        return False
